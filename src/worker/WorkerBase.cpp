#include "native/worker/WorkerBase.hpp"
#include "native/Error.hpp"
#include "native/helper/trace.hpp"

#include <mutex>

namespace {
// TODO find a way to remove mutex
std::mutex mutexInProcess;
}

namespace native {

WorkerBase::WorkerBase(std::shared_ptr<Loop> iLoop) : _loop(iLoop) {
  NNATIVE_FCALL();
  NNATIVE_ASSERT(_loop);
  _uvWork.data = nullptr;
}

WorkerBase::~WorkerBase() {
  NNATIVE_FCALL();
  // Check if the destructor is called with pending job
  NNATIVE_ASSERT(this->_uvWork.data == nullptr);
}

void WorkerBase::enqueue() {
  NNATIVE_FCALL();
  std::lock_guard<std::mutex> guard(mutexInProcess);
  NNATIVE_ASSERT(!_instance);
  // Make sure that the worker is not in progress
  NNATIVE_ASSERT(this->_uvWork.data == nullptr);
  this->_uvWork.data = this;
  this->_instance = getInstance();

  if (uv_queue_work(_loop->get(), &_uvWork, WorkerBase::Worker, WorkerBase::WorkerAfter) != 0) {
    NNATIVE_DEBUG("Error in uv_queue_work");
    _uvWork.data = nullptr;
    this->_instance.reset();
    throw Exception("uv_queue_work");
  }
  NNATIVE_DEBUG("Enqueued");
}

void WorkerBase::Worker(uv_work_t *iHandle) {
  NNATIVE_FCALL();

  NNATIVE_ASSERT(iHandle->data != nullptr);
  WorkerBase *currobj = static_cast<WorkerBase *>(iHandle->data);

  currobj->executeWorker();
}

void WorkerBase::WorkerAfter(uv_work_t *iHandle, int iStatus) {
  NNATIVE_FCALL();
  std::shared_ptr<WorkerBase> currInst;
  {
    std::lock_guard<std::mutex> guard(mutexInProcess);
    NNATIVE_ASSERT(iHandle->data != nullptr);
    NNATIVE_DEBUG("iStatus: " << iStatus);

    WorkerBase *currobj = static_cast<WorkerBase *>(iHandle->data);
    iHandle->data = nullptr;

    NNATIVE_ASSERT(currobj->_instance);

    // Save a instance copy do not call the destructor
    currInst = currobj->_instance;
    currInst->_instance.reset();

    currInst->executeWorkerAfter(iStatus);
  }
  currInst->closeWorker();
}

} /* namespace native */
