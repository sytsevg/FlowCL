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

// Standard includes
#include <thread>

// OpenCL includes
#define __CL_ENABLE_EXCEPTIONS
#include "CL/cl.hpp"

// Project includes
#include "FrameworkTypes.hpp"
#include "MemoryManager.hpp"
#include "Synchronization.hpp"
#include "Operation.hpp"
#include "Device.hpp"
#include "Edge.hpp"

namespace Dataflow
{

	MemoryManager::MemoryManager() { }

	MemoryManager::~MemoryManager()
	{
		for( Device* dev : m_devices )
		{
			delete dev;
		}

		for( const Memory* mem : m_device_memories )
		{
			delete mem;
		}

		for( Edge* edge : m_edges )
		{
			delete edge;
		}

		for( Operation* op : m_operations )
		{
			delete op;
		}

		for( SyncBarrier* sb : m_sync_barriers )
		{
			delete sb;
		}

		for( void* raw : m_raw_memories )
		{
			::operator delete( raw );
		}
	}

	const Memory* MemoryManager::NewMemory( size_t size,
									        const void* ptr_mem,
									        size_t id )
	{
		const Memory* dm = new Memory( size, (void* const)ptr_mem, id );

		m_device_memories.push_back( dm );

		return dm;
	}

	Device* MemoryManager::NewDevice( const cl::Device& cl_device,
									  const cl::Context& cl_context, 
									  const cl::Program& cl_program )
	{
		Device* dev = new Device( cl_device, cl_context, cl_program );

		m_devices.push_back( dev );

		return dev;
	}

	Operation* MemoryManager::NewOperation( const Device* dev,
										    const std::string& str_kernel_name,
										    SyncBarrier* sb_in,
										    SyncBarrier* sb_out )
	{
		Operation* op = new Operation( dev, str_kernel_name, sb_in, sb_out );

		m_operations.push_back( op );

		return op;
	}

	Operation* MemoryManager::NewOperation( const Device* dev,
										    void (CL_CALLBACK *user_func)(void*),
										    SyncBarrier* sb_in,
										    SyncBarrier* sb_out )
	{
		Operation* op = new Operation( dev, user_func, sb_in, sb_out );

		m_operations.push_back( op );

		return op;
	}

	Edge* MemoryManager::NewEdge( const Operation* op_input,
								  const Operation* op_output,
								  const Memory* dm,
								  const EdgeType type,
								  CopyInterval copy_interval)
	{
		Edge* e = new Edge( op_input, op_output, dm, type, copy_interval );

		m_edges.push_back( e );

		return e;
	}

	void* MemoryManager::NewRawMemory( size_t size )
	{
		void* raw = ::operator new( size );

		m_raw_memories.push_back( raw );

		return raw;
	}

	void* MemoryManager::NewRawMemory( size_t size, const void* data )
	{
		void* raw = ::operator new( size );

		memcpy( raw, data, size );

		m_raw_memories.push_back( raw );

		return raw;
	}

	SyncBarrier* MemoryManager::NewSyncBarrier()
	{
		SyncBarrier* sb = new SyncBarrier();

		m_sync_barriers.push_back( sb );

		return sb;
	}

} // End namespace Processing Framework
