/* =============================================================================
 *
 * Title:       Inter-Process communication methods
 * Author:      Felix Niederwanger
 * License:     MIT (http://opensource.org/licenses/MIT)
 * Description: Library, code file
 * =============================================================================
 */

#include <string>
#include <sstream>
#include <fstream>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "ipc.hpp"

using namespace std;

SharedMemory::SharedMemory() {
	this->shm_key = 0;
	this->shmid = 0;
	this->mem = NULL;
	this->_attrs = 0;
	this->_size = 0;
	this->_detachOnDestruction = true;
	this->_deleteOnDestruction = false;
	this->_created = false;
}

SharedMemory::SharedMemory(int key) {
	this->shm_key = key;
	this->shmid = 0;
	this->mem = NULL;
	this->_attrs = 0;
	this->_size = 0;
	this->_detachOnDestruction = true;
	this->_deleteOnDestruction = false;
	this->_created = false;
}

SharedMemory::SharedMemory(int key, size_t size, int attr) {
	this->shm_key = key;
	this->shmid = 0;
	this->mem = NULL;
	this->_attrs = 0;
	this->_size = 0;
	this->_detachOnDestruction = true;
	this->_created = false;
	this->attach(size, attr);
	this->_deleteOnDestruction = this->_created;
}

SharedMemory::SharedMemory(const SharedMemory &ref) {
	this->shm_key = ref.shm_key;
	this->shmid = 0;
	this->mem = NULL;
	this->_attrs = 0;
	this->_size = 0;
	this->_created = false;
	this->_detachOnDestruction = true;
	this->_deleteOnDestruction = false;
	this->attach(ref._size, ref._attrs);
}

SharedMemory::~SharedMemory() {
	const int shm_key = this->shm_key;
	const int shmid = this->shmid;
	const size_t size = this->_size;

	if(this->_detachOnDestruction || this->_deleteOnDestruction) {
		// Check if attached, and if so, detach shared memory
		try {
			if(this->mem != NULL)
				this->detach();
		} catch (...) {
			// Swallow exception in destructor
		}
	}
	if(this->_deleteOnDestruction) {
		if(shmid <= 0)
			SharedMemory::destroy(shm_key, size);
		else {
			if (::shmctl(shmid, IPC_RMID, NULL) < 0) {
				// Possible error handling
			}
		}
	}
}


void *SharedMemory::createNew(const int key, const size_t size, int attr) {
	if(key < 0) throw IPCException("Illegal shared memory key");

	// Create shared memory
	const int shmid = shmget(key, size, attr | IPC_CREAT);
	if(shmid < 0)
		throw IPCException("Error creating SharedMemory");

	// Attach shared memory
	void *mem = shmat(shmid, NULL, 0);
	if(mem == NULL || (long)mem < 0)
		throw IPCException("Attaching shared memory failed");

	return mem;
}

void SharedMemory::setDetachOnDispose(bool enabled) {
	this->_detachOnDestruction = enabled;
}

bool SharedMemory::detachOnDispose(void) const {
	return this->_detachOnDestruction;
}

void SharedMemory::setDeleteOnDispose(bool enabled) {
	this->_deleteOnDestruction = enabled;
}

bool SharedMemory::isCreated(void) const {
	return this->_created;
}

void SharedMemory::detach(void *mem) {
	if(mem == NULL || (long)mem < 0L) return;

	int ret = ::shmdt(mem);
	if(ret < 0) throw IPCException("Detaching shared memory failed");

}

void SharedMemory::setKey(int key) {
	this->shm_key = key;
}

int SharedMemory::key(void) const {
	return this->shm_key;
}
int SharedMemory::id(void) const {
	return this->shmid;
}

void *SharedMemory::get(void) const {
	if(this->shmid <= 0)
		return NULL;
	else
		return this->mem;
}

void *SharedMemory::create(size_t size, int attr) {
	return this->create(this->shm_key, size, attr);
}

void *SharedMemory::create(int shm_key, size_t size, int attr) {
	if(shm_key <= 0) throw IPCException("Illegal shared memory key");
	if(this->isAttached()) throw IPCException("Cannot create shared memory while already one is attached to this class object");

	int shmid = shmget(shm_key, size, attr | IPC_CREAT);
	if(shmid < 0)
		throw IPCException("Error creating SharedMemory");
	this->shmid = shmid;

	// Attach shared memory
	this->mem = shmat(this->shmid, NULL, 0);
	if(!this->isAttached())
		throw IPCException("Attaching shared memory failed");

	this->_created = true;
	this->_attrs = attr;
	this->_size = size;
	return this->mem;
}


bool SharedMemory::isAttached(void) const {
	if(this->mem == NULL) return false;
	//if(this->mem <= 0) return false;
	return true;
}


void *SharedMemory::attach(const size_t size, int attr) {
	if(shm_key < 0) throw IPCException("Illegal shared memory key");
	if(this->isAttached()) throw IPCException("Cannot create shared memory while already one is attached to this class object");
	if(size > SharedMemory::maxSize()) throw IPCException("Cannot allocate more memory than allowed by system");

	int shmid;
	if(shm_key == 0)
		shmid = ::shmget(shm_key, size, attr | IPC_CREAT);
	else
		shmid = ::shmget(shm_key, size, attr | IPC_CREAT | IPC_EXCL);
	if(shmid < 0) {
		// Try just to get shared memory
		if(errno == EEXIST) {
			shmid = ::shmget(shm_key, size, attr);
			if(shmid < 0)
				throw IPCException("Error creating SharedMemory");
			this->_created = false;
		} else
			throw IPCException("Error creating SharedMemory");
	} else
		this->_created = true;

	this->shmid = shmid;
	// Attach shared memory
	this->mem = shmat(this->shmid, NULL, 0);
	if(!this->isAttached())
		throw IPCException("Attaching shared memory failed");

	this->_attrs = attr;
	this->_size = size;
	return this->mem;
}

SharedMemory SharedMemory::attach(const int id, const size_t size) {
	SharedMemory shm;

	shm._attrs = 0;
	shm._size = size;
	shm._created = false;
	shm._deleteOnDestruction = false;
	shm._detachOnDestruction = true;
	shm.shm_key = 0;
	shm.shmid = id;
	shm.mem = shmat(id, NULL, 0);

	return shm;
}

SharedMemory *SharedMemory::attachNew(const int id, const size_t size) {
	SharedMemory *shm = new SharedMemory();

	shm->_attrs = 0;
	shm->_size = size;
	shm->_created = false;
	shm->_deleteOnDestruction = false;
	shm->_detachOnDestruction = true;
	shm->shm_key = 0;
	shm->shmid = id;
	shm->mem = shmat(id, NULL, 0);

	return shm;
}

void SharedMemory::detach(void) {
	if(!this->isAttached()) throw IPCException("Shared-memory not attached");

	int ret = ::shmdt(this->mem);
	if(ret < 0) throw IPCException("Detaching shared memory failed");
	this->mem = NULL;
}

bool SharedMemory::destroy(const int key, const size_t size, int attr) {
	if(key <= 0) throw IPCException("Illegal shared-memory key");

	const int shmid = ::shmget(key, size, attr);
	if(shmid < 0)
		throw IPCException("Error getting SharedMemory");

	int ret = shmctl(shmid, IPC_RMID, NULL);
	return ret == 0;
}

void SharedMemory::destroy(void) {
	const int shmid = this->shmid;
	if(shmid <= 0) throw IPCException("Not attached to shared memory segment");
	if(this->isAttached()) {
		if(::shmdt(this->mem) < 0)
			throw IPCException("Error detaching shared memory");
	}
	this->mem = NULL;
	this->shmid = 0;
	if (::shmctl(shmid, IPC_RMID, NULL) < 0)
		throw IPCException("Destroying Shared memory failed");
}

void *SharedMemory::operator*(void) const {
	return this->get();
}


void SharedMemory::stats(struct shmid_ds *buf) const {
	if (!this->shm_ctl(IPC_STAT, buf))
		throw IPCException("Access to shared memory failed");
}

struct ::shmid_ds SharedMemory::stats(void) const {
	struct ::shmid_ds buf;
	this->stats(&buf);
	return buf;
}


int SharedMemory::nAttached(void) const {
	struct ::shmid_ds buf;
	this->stats(&buf);
	return buf.shm_nattch;
}

ipc_perm SharedMemory::permissions(void) const {
	struct ::shmid_ds buf;
	this->stats(&buf);
	return buf.shm_perm;
}

size_t SharedMemory::size(void) const {
	struct ::shmid_ds buf;
	this->stats(&buf);
	return buf.shm_segsz;
}


bool SharedMemory::shm_ctl(int cmd) const {
	if(this->shmid <= 0) throw IPCException("Shared-memory ID not defined");

	int ret = shmctl(this->shmid, cmd, NULL);
	return ret == 0;
}

bool SharedMemory::shm_ctl(int cmd, struct ::shmid_ds *buf) const {
	if(this->shmid <= 0) throw IPCException("Shared-memory ID not defined");

	int ret = shmctl(this->shmid, cmd, buf);
	return ret == 0;
}

bool SharedMemory::exists(const int shm_key, size_t size) {
	const int id = ::shmget(shm_key, size, 0600);
	if(id >= 0) return true;
	else {
		const int no_err = errno;
		if (no_err == ENOENT || no_err == EACCES) return false;
		else {
			throw IPCException("Unknown error querying shared memory");
		}
	}
}

size_t SharedMemory::maxSize(void) {
	ifstream in("/proc/sys/kernel/shmmax");
	if(!in.is_open()) return -1;
	size_t ret;
	in >> ret;
	in.close();
	return ret;
}

Semaphore::Semaphore(int key, int attr) {
	this->semkey = key;
	this->semid = ::semget(key, 1, IPC_CREAT | attr);
	if (this->semid < 0)
		throw IPCException("Error creating semaphore");


}

Semaphore::~Semaphore() {

}

int Semaphore::key(void) const { return this->semkey; }
int Semaphore::id(void) const { return this->semid; }

bool Semaphore::destroy(const int key, const int attr) {
	const int semid = ::semget(key, 1, attr);
	if(semid <= 0) return false;
	if (::semctl(semid, 0, IPC_RMID) < 0)
		return false;
	else
		return true;
}

void Semaphore::destroy() {
	if(this->semid < 0) throw IPCException("Illegal semaphore id");

	if (::semctl(this->semid, 0, IPC_RMID) < 0)
		throw IPCException("Error destroying semaphore");

	this->semid = 0;
}

void Semaphore::setValue(int value) const {
	if(this->semid < 0) throw IPCException("Illegal semaphore id");

	if (::semctl(this->semid, 0, SETVAL, value) < 0)
		throw IPCException("Error setting value of semaphore");
}


int Semaphore::getValue() const {
	if(this->semid < 0) throw IPCException("Illegal semaphore id");

	int semValue = ::semctl(this->semid, 0, GETVAL);
	if (semValue < 0)
		throw IPCException("Error getting value of semaphore");
	return semValue;
}

int Semaphore::operator*(void) const {
	return this->getValue();
}



void Semaphore::increase(int count) const {
	if(this->semid < 0) throw IPCException("Illegal semaphore id");

	if(count == 0) return;
	else if(count < 0) throw IPCException("Semaphore counter cannot be negative");
	struct sembuf sop;

	sop.sem_num = 0;
	sop.sem_op = count;
	sop.sem_flg = 0;
	if (::semop(this->semid, &sop, 1) < 0)
		throw IPCException("Error increasing semaphore");
}


void Semaphore::decrease(int count) const {
	if(this->semid < 0) throw IPCException("Illegal semaphore id");

	if(count == 0) return;
	else if(count < 0) throw IPCException("Semaphore counter cannot be negative");
	struct sembuf sop;

	sop.sem_num = 0;
	sop.sem_op = -count;
	sop.sem_flg = 0;
	if (::semop(this->semid, &sop, 1) < 0)
		throw IPCException("Error increasing semaphore");
}


void Semaphore::aquire(int count) {
	this->decrease(count);
}

void Semaphore::release(int count) {
	this->increase(count);
}
