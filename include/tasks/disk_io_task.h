/*
 * Copyright (c) 2013-2014 Andreas Pohl <apohl79 at gmail.com>
 *
 * This file is part of libtasks.
 * 
 * libtasks is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * libtasks is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with libtasks.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TASKS_IO_TASK_H_
#define _TASKS_IO_TASK_H_

#include <tasks/task.h>
#include <tasks/ev_wrapper.h>
#include <tasks/tools/buffer.h>
#include <future>

namespace tasks {

class worker;

class disk_io_task : public task {
public:
    disk_io_task(int fd, int events, tools::buffer* buf);
    disk_io_task(int fd, int events, tools::buffer* buf, std::streamsize* ret);
    virtual ~disk_io_task();

    inline std::string get_string() const {
        std::ostringstream os;
        os << "disk_io_task(" << m_fd << ":" << m_events << ")";
        return os.str();
    }

    /*
     * Return the number of bytes written to or read from the file. This method waites for
     * io operation to complete.
     */
    inline std::streamsize bytes(bool block = false) const {
        m_handle.wait();
        return m_bytes;
    }

    virtual bool handle_event(worker* worker, int events);
    virtual void start_watcher(worker* worker) {}
    virtual void stop_watcher(worker* worker) {}

    virtual void dispose(worker* worker);

    static void add_task(disk_io_task* task) {
        assert(nullptr != task);
        task->op();
    }

private:
    int m_fd = -1;
    int m_events = EV_UNDEF;
    tools::buffer* m_buf = nullptr;
    std::streamsize m_bytes = -1;
    std::streamsize* m_ret = nullptr;
    std::future<void> m_handle;

    void op();
};

} // tasks

#endif // _TASKS_IO_TASK_H_