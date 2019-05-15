/* =============================================================================
 *
 * Title:         Inter-process communication module
 * Author:        Felix Niederwanger
 *
 * =============================================================================
 */

#ifndef _LINUX_IPC_HPP_
#define _LINUX_IPC_HPP_

#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <exception>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>

class IPCException;
class SharedMemory;
class Semaphore;

/** General IPC exception class */
class IPCException : public std::exception {
	public:
		explicit IPCException(const char* message) : msg_(message) {}
		explicit IPCException(const std::string& message) : msg_(message) {}
		virtual ~IPCException() throw (){}
		virtual const char* what() const throw (){ return msg_.c_str(); }

protected:
    std::string msg_;
};


/**
 * Implements a shared memory segment
 */
class SharedMemory {
private:
	/** Shared memory ID */
	int shmid;

	/** Shared-memory key */
	int shm_key;

	/** Memory pointer */
	void *mem;

	/** Flag indicating if we detach on destruction */
	bool _detachOnDestruction;

	/** Flag indicating if we detach on destruction */
	bool _deleteOnDestruction;

	/** Created flag */
	bool _created;

	/** Stored attributes */
	int _attrs;

	/** Size of the shared memory */
	size_t _size;

protected:

	/** Execute shared memory command on this shared memory segment
	  * @param cmd Command to be executed (SHM_LOCK, SHM_UNLOCK, IPC_STAT, IPC_SET, IPC_RMID)
	  */
	bool shm_ctl(int cmd) const;

	/** Execute shared memory command on this shared memory segment
	  * @param cmd Command to be executed (SHM_LOCK, SHM_UNLOCK, IPC_STAT, IPC_SET, IPC_RMID)
	  * @param buf Shared memory stats buffer, if needed for the command
	  * @returns true if the access is successfull, false if it fails
	  */
	bool shm_ctl(int cmd, struct ::shmid_ds *buf) const;

public:
	SharedMemory();
	SharedMemory(int key);
	SharedMemory(int key, size_t size, int attr = 0600);
	SharedMemory(const SharedMemory &ref);

	virtual ~SharedMemory();

	void setKey(int key);
	int key(void) const;
	int id(void) const;

	/**
	 * Enable or disable the detach on dispose routine
	 * @param enabled if true, the shared memory will be detached on disposal, if false if won't be detached. Default value is true
	 * */
	void setDetachOnDispose(bool enabled= true);
	/**
	 * @return true if the detach on dispose flag is set
	 */
	bool detachOnDispose(void) const;

	/**
	 * Enable or disable the delete on dispose routine
	 * @param enabled if true, the shared memory will be detached on disposal, if false if won't be detached. Default value is true
	 * */
	void setDeleteOnDispose(bool enabled = true);

	/**
	 * @returns true, if this class has created the shared memory
	 */
	bool isCreated(void) const;


	/**
	 * Get the shared memory stats
	 * @returns Shared memory stats struct
	 * @throws IPCException If the access to the shared memory fails
	 */
	struct shmid_ds stats(void) const;


	/**
	 * Get the shared memory stats and write them in the given struct
	 * @param buf shared memory stats struct where to write
	 * @throws IPCException If the access to the shared memory fails
	 */
	void stats(struct shmid_ds *buf) const;

	/**
	 * @returns number of attached clients to the shared memory
	 */
	int nAttached(void) const;

	/**
	 * @returns permissions to the shared memory segment
	 */
	ipc_perm permissions(void) const;

	/** Size of the attached segment */
	size_t size(void) const;

	/**
	 * Create new shared memory segment
		  @param shm_key Key of the new Shared memory segment
		  @param size Size in bytes of the new shared memory segment
		  @param attr Attributes of the the shared memory. IPC_CREAT will be added to this. Default value is 0600
		  @throws IPCException on an error
	 */
	void *create(int shm_key, size_t size, int attr = 0600);


	/**
	  * Create new shared memory segment
	  @param size Size in bytes of the new shared memory segment
	  @param attr Attributes of the the shared memory. IPC_CREAT will be added to this. Default value is 0600
	  @throws IPCException on an error
	  */
	void *create(size_t size, int attr = 0600);

	/**
	 * Attach to shared memory with the internal key
	 */
	void *attach(const size_t size, int attr = 0600);

	/**
	 * @returns true if the shared memory object is attached
	 */
	bool isAttached(void) const;

	/**
	 * Detach the current shared memory segment
	 */
	void detach(void);

	/**
	 * Detach and delete current shared memory segment
	 */
	void destroy(void);


	/** Get the actual memory segment */
	void *get(void) const;
	/** Get the actual memory segment */
	void *operator*(void) const;

	/**
	 * Create or attach to the given key as shared memory
	 * @param key Shared memory key
	 * @param size Size of the shared memory segment in bytes
	 * @param attr Attribute of the shared memory segment
	 * @throws IPCException Thrown if something went wrong
	 * @return pointer to the shared memory
	 */
	static void *createNew(const int key, const size_t size, int attr = 0600);
	/**
	 * Detach the given pointer to a shared memory
	 * @throws IPCException Thrown if something went wrong
	 */
	static void detach(void *mem);

	/** Destroy the given shared memory segment
	  * @param shm_key Key of the shared memory segment to be destroyed
	  * @param size Size of the shared memory segment to be destroyed. Must match the size, so delete some space is not possible
	  * @param attr Attribute when getting shared memory segment. Dfault: 0600
	  * @returns true if the the segment has been destroyed, false if an error occurred
	  */
	static bool destroy(const int shm_key, const size_t size, int attr = 0600);

	/**
	 * Checks if the given shared memory key exists
	 */
	static bool exists(const int shm_key, size_t size);

	/** Read out the maximum available size for a shared memory segment */
	static size_t maxSize(void);

	/**
	 * Attach to shared memory with the given id
	 */
	static SharedMemory attach(const int id, const size_t size);
	/**
	 * Attach to shared memory with the given id
	 */
	static SharedMemory *attachNew(const int id, const size_t size);

	/** Locks the mutex of this shared memory. Blocks until the locks is yielded */
	void lock(void);
	/** Unlocks the mutex of this shared memory */
	void unlock(void);
};


class Semaphore {
private:
	/** Semaphore id */
	int semid;

	/** Semaphore key */
	int semkey;
public:
	/** Attach to the given semaphore id
	 * @param key Key of the semaphore to be used
	 * @param attr Attribute for the semaphore. Default is 0600
	 * */
	Semaphore(int key, int attr = 0600);

	virtual ~Semaphore();

	/** Get the key of the semaphore */
	int key(void) const;
	/** Get the id of the semaphore */
	int id(void) const;

	/** Set the value of the semaphore
	 * @param value Value to be set
	 * */
	void setValue(int value) const;

	/**
	 * Get the value of the semaphore
	 * @return value of the semaphore
	 */
	int getValue() const ;

	/** @return value of the semaphore */
	int operator*(void) const;

	/**
	 * Increase semaphore by the given value
	 * @param count increase counter. Default is 1
	 */
	void increase(int count = 1) const;
	/**
	 * Decrease semaphore by the given value
	 * @param count increase counter. Default is 1
	 */
	void decrease(int count = 1) const;

	/** Destroys this semaphore */
	void destroy(void);

	/** Acquire resources from semaphore. Blocks until the given count is available
	 * @param count Counter indicating how many resources should be acquired. Default value is 1
	 */
	void aquire(int count = 1);

	/** Release resources to semaphore. Blocks until the given count is available
	 * @param count Counter indicating how many resources should be released. Default value is 1
	 */
	void release(int count = 1);

	/** Destroy the given semaphore
	  * @param key Key of the semaphore to be destroyed
	  * @param attr Attribute with witch the semaphore is accessed
	  * @returns true if the action was successful, otherwise false. If it fails, errno is set
	  */
	static bool destroy(const int key, const int attr = 0600);
};

#endif
