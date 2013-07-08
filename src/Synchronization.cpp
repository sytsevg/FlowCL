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

	Author: S.D.M. van Geldermalsen
*/

// Project includes
#include "FrameworkTypes.hpp"
#include "Synchronization.hpp"

namespace Dataflow
{

	SyncBarrier::SyncBarrier()
							: m_barrier_size(0),
							  m_thread_count(0)
	{
	}

	SyncBarrier::~SyncBarrier()	{ }

	SyncBarrier* SyncBarrier::Add()
	{
		m_barrier_size++;
		return this;
	}

	SyncBarrier* SyncBarrier::Remove()
	{
		m_barrier_size--;
		return this;
	}

	void SyncBarrier::Wait()
	{
		std::unique_lock<std::mutex> lock( m_mutex_cond );

		m_thread_count++;

		if( m_thread_count == m_barrier_size )
		{
			m_thread_count = 0;
			m_cond.notify_all();
		}
		else
		{
			m_cond.wait( lock );
		}
	}

	const std::string& SyncBarrier::GetDebugInfo()
	{
		std::stringstream ss;

		ss << this << " size: " << m_barrier_size;

		return m_debug = ss.str();
	}

	SyncStart::SyncStart()
						: m_num_wait_cond(0)
	{

	}

	SyncStart::~SyncStart()	{ }

	void SyncStart::Wait()
	{
		std::unique_lock<std::mutex> lock( m_mutex_cond );
		m_num_wait_cond++;
		m_cond.wait( lock );
	}

	void SyncStart::Start( size_t num_threads )
	{
		bool threads_locked = true;

		// Wait for all the created threads to reach start lock
		while( threads_locked )
		{
			std::unique_lock<std::mutex> lock( m_mutex_cond );
			threads_locked = ( m_num_wait_cond != num_threads );
		}

		// Resume all waiting delay start threads
		m_cond.notify_all();
	}

} // End namespace Dataflow