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
 Memory Manager

 Simple memory manager for objects created by new

 Author: S.D.M. van Geldermalsen

 Date: 11-12-2012
*/

#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

namespace Dataflow
{

	class MemoryManager
	{

	public:

		MemoryManager();
	   ~MemoryManager();

	    const Memory* NewMemory( size_t size,
						               const void* ptr_mem,
						               size_t id );

		Device* NewDevice( const cl::Device& cl_device,
						   const cl::Context& cl_context, 
						   const cl::Program& cl_program );

		Operation* NewOperation( const Device* dev,
								 const std::string& str_kernel_name,
								 SyncBarrier* sb_in,
								 SyncBarrier* sb_out );

		Operation* NewOperation( const Device* dev,
								 void (CL_CALLBACK *user_func)(void*),
								 SyncBarrier* sb_in,
								 SyncBarrier* sb_out );

		Edge* NewEdge( const Operation* op_input,
					   const Operation* op_output,
					   const Memory* dm,
					   const EdgeType type,
					   CopyInterval copy_interval );

		SyncBarrier* NewSyncBarrier();

		void* NewRawMemory( size_t size );

		void* NewRawMemory( size_t size, const void* data );

	private:

		std::vector<Device*>             m_devices;
		std::vector<Operation*>          m_operations;
		std::vector<Edge*>               m_edges;
		std::vector<const Memory*> m_device_memories;
		std::vector<SyncBarrier*>        m_sync_barriers;
		std::vector<void*>               m_raw_memories;

	}; // end of class Memory Manager

} // End namespace Dataflow

#endif // MEMORY_MANAGER_HPP