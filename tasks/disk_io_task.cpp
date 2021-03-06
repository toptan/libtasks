/*
 * Copyright (c) 2013-2014 ADTECH GmbH
 * Licensed under MIT (https://github.com/adtechlabs/libtasks/blob/master/COPYING)
 *
 * Author: Andreas Pohl
 */

#include <tasks/worker.h>
#include <tasks/disk_io_task.h>
#include <tasks/logging.h>
#include <unistd.h>

namespace tasks {

disk_io_task::disk_io_task(int fd, int events, tools::buffer* buf) : m_fd(fd), m_events(events), m_buf(buf) {
    tdbg(get_string() << ": ctor" << std::endl);
}

disk_io_task::~disk_io_task() { tdbg(get_string() << ": dtor" << std::endl); }

std::shared_future<std::streamsize> disk_io_task::op() {
    // run the io op in a separate thread
    std::promise<std::streamsize> bytes_promise;
    std::async(std::launch::async, [this, &bytes_promise] {
        switch (m_events) {
            case EV_READ:
                tdbg(get_string() << ": calling read()" << std::endl);
                m_bytes = read(m_fd, m_buf->ptr_write(), m_buf->to_write());
                tdbg(get_string() << ": read() returned " << m_bytes << std::endl);
                if (m_bytes > 0) {
                    m_buf->move_ptr_write(m_bytes);
                }
                break;
            case EV_WRITE:
                tdbg(get_string() << ": calling write()" << std::endl);
                m_bytes = write(m_fd, m_buf->ptr_read(), m_buf->to_read());
                tdbg(get_string() << ": write() returned " << m_bytes << std::endl);
                break;
            default:
                m_bytes = -1;
                tasks_exception e(tasks_error::DISKIO_INVALID_EVENT,
                                  get_string() + std::string(": events has to be either EV_READ or EV_WRITE"));
                set_exception(e);
        }
        bytes_promise.set_value(m_bytes);
        // fire an event
        event e = {this, m_events};
        worker::add_async_event(e);
    });
    return bytes_promise.get_future();
}

bool disk_io_task::handle_event(worker* /* worker */, int /* events */) {
    tdbg(get_string() << ": handle_event" << std::endl);
    return false;
}

void disk_io_task::dispose(worker* worker) {
    if (nullptr == worker) {
        worker = dispatcher::instance()->get_worker_by_task(this);
    }
    worker->exec_in_worker_ctx([this](struct ev_loop* /* loop */) {
        tdbg(get_string() << ": disposing disk_io_task" << std::endl);
        delete this;
    });
}

}  // tasks
