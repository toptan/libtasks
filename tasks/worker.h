/*
 * Copyright (c) 2013 Andreas Pohl <apohl79 at gmail.com>
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

#ifndef _TASKS_WORKER_H_
#define _TASKS_WORKER_H_

#include <tasks/dispatcher.h>
#include <tasks/task.h>
#include <tasks/logging.h>
#include <tasks/ev_wrapper.h>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <sstream>
#include <cassert>

namespace tasks {

	class task;

	// Needed to use std::unique_ptr<>
	class loop_wrapper {
	public:
		struct ev_loop *loop;
	};

	// Signals to enter the leader thread context
	typedef std::function<void(struct ev_loop*)> task_func;

	struct task_func_queue {
		std::queue<task_func> queue;
		std::mutex mutex;
	};

	// Put all queued events into a queue instead of handling them
	// directly from handle_io_event as multiple events can fire and
	// we want to promote the next leader after the ev_loop call
	// returns to avoid calling it from multiple threads.
	struct event {
		tasks::task* task;
		int revents;
	};
		
	class worker {
	public:
		worker(int id);
		virtual ~worker();

		inline int get_id() const {
			return m_id;
		}

		inline std::string get_string() const {
			std::ostringstream os;
			os << "worker(" << m_id << ")";
			return os.str();
		}

		// Executes task_func directly if called in leader thread
		// context or delegates it. Returns true when task_func has
		// been executed.
		inline bool signal_call(task_func f) {
			if (m_leader) {
				// The worker is the leader, now execute the functor
				f(m_loop->loop);
				return true;
			} else {
				task_func_queue* tfq = (task_func_queue*) m_signal_watcher.data;
				std::lock_guard<std::mutex> lock(tfq->mutex);
				tfq->queue.push(f);
				ev_async_send(ev_default_loop(0), &m_signal_watcher);
				return false;
			}
		}

		inline void set_event_loop(std::unique_ptr<loop_wrapper> loop) {
			m_loop = std::move(loop);
			m_leader.store(true);
			ev_set_userdata(m_loop->loop, this);
			m_work_cond.notify_one();
		}

		inline void terminate() {
			m_term.store(true);
			m_work_cond.notify_one();
		}

		inline void add_event(event e) {
			m_events_queue.push(e);
		}
		
		void handle_io_event(ev_io* watcher, int revents);
		void handle_timer_event(ev_timer* watcher);
		
	private:
		int m_id;
		std::thread m_thread;
		std::unique_ptr<loop_wrapper> m_loop;
		std::atomic<bool> m_leader;
		std::atomic<bool> m_term;
		std::mutex m_work_mutex;
		std::condition_variable m_work_cond;
		std::queue<event> m_events_queue;

		// Every worker has an async watcher to be able to call
		// into the leader thread context.
		ev_async m_signal_watcher;

		inline void promote_leader() {
			std::shared_ptr<worker> w = dispatcher::get_instance()->get_free_worker();
			if (nullptr != w) {
				// If we find a free worker, we promote it to the next
				// leader. This thread stays leader otherwise.
				m_leader.store(false);
				w->set_event_loop(std::move(m_loop));
			}
		}

		void run();
	};
	
	/* CALLBACKS */
	template<typename EV_t>
	static void tasks_event_callback(struct ev_loop* loop, EV_t w, int e) {
		worker* worker = (tasks::worker*) ev_userdata(loop);
		assert(nullptr != worker);
		task* task = (tasks::task*) w->data;
		task->stop_watcher(worker);
		event event = {task, e};
		worker->add_event(event);
	}

	static void tasks_async_callback(struct ev_loop* loop, ev_async* w, int events) {
		worker* worker = (tasks::worker*) ev_userdata(loop);
		assert(nullptr != worker);
		task_func_queue* tfq = (tasks::task_func_queue*) w->data;
		assert(nullptr != tfq);
		std::lock_guard<std::mutex> lock(tfq->mutex);
		// Execute all queued functors
		while (!tfq->queue.empty()) {
			assert(worker->signal_call(tfq->queue.front()));
			tfq->queue.pop();
		}
	}

} // tasks

#endif // _TASKS_WORKER_H_