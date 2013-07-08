/*
    This file is part of FlowCL.

    FlowCL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FlowCL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FlowCL.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
  Synchronization

  SharedSyncObject

	Simple object that implements barrier synchronization
	Current implementation uses the boost::barrier

  SyncStart

	One time synchronization mechanism to syncronize all new threads  

 Author: S.D.M. van Geldermalsen

 Date: 22-09-2012
 */

#ifndef SYNCHRONIZATION_HPP
#define SYNCHRONIZATION_HPP

// Standard includes
#include <mutex>
#include <condition_variable>

namespace Dataflow
{

	class SyncBarrier
	{

	public:

		SyncBarrier();
		~SyncBarrier();

		// Add a thread to the barrier
		SyncBarrier* Add();

		// Remove a thread from the barrier
		SyncBarrier* Remove();

		// Blocking wait until number of threads reach count
		void Wait();

		const std::string& GetDebugInfo();

	private:

		// Mutex to access condition variable
		std::mutex				m_mutex_cond;
		// Condition variable to wait on
		std::condition_variable m_cond;
		// Number of threads of barrier
		size_t					m_barrier_size;
		// Number of threads that have reached barrier
		size_t					m_thread_count;

		std::string				m_debug;

	}; // End class SyncBarrier

	class SyncStart
	{

	public:

		SyncStart();
		~SyncStart();

		// Wait to start execution
		void Wait();

		// Unloack all waiting threads
		void Start( size_t num_threads );

	private:

		// Mutex to access condition variable
		std::mutex               m_mutex_cond;
		// Condition variable to wait on
		std::condition_variable  m_cond;
		// Number of threads waiting on condition variable
		size_t                   m_num_wait_cond;

	}; // End class SyncStart

} // End namespace Dataflow

#endif //SYNCHRONIZATION_HPP