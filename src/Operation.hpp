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
 Abstract Operation Header

 Represents an abstract object that is connected to a series of input edges and 
 output edges. Performs a given computation on a selected device, when all 
 input edges are available. On completion, fires output edges.

 Author: S.D.M. van Geldermalsen

 Date: 13-09-2012
*/

#ifndef OPERATION_HPP
#define OPERATION_HPP

namespace Dataflow
{

	class Operation
	{

	public:

		Operation( const Device* dev,
				   const std::string& str_kernel_name,
				   SyncBarrier* sb_in,
				   SyncBarrier* sb_out );
		Operation( const Device* dev,
				   void (CL_CALLBACK *user_func)(void*),
				   SyncBarrier* sb_in,
				   SyncBarrier* sb_out );
		~Operation();

		void SetKernelArgMemory( size_t index,
			                     const Memory* dev_mem,
						         const Edge* edge );

		void SetKernelArgConstant( size_t index,
			                       size_t size,
								   const char* type,
								   const void* arg );

		void SetWorkSize( const cl::NDRange& global,
						  const cl::NDRange& local );

		void AddInputEdge( const Edge* edge );
		void AddOutputEdge( const Edge* edge );

		const Edge* GetEdgeAtArgIndex( size_t index );

		SyncBarrier* GetInputSync();
		SyncBarrier* GetOutputSync();

		void SetInputSync( SyncBarrier* sb_in );
		void SetOutputSync( SyncBarrier* sb_out );

		const Device* GetDevice() const;

		void ThreadStart( SyncStart* start, bool* execute );

		const std::string& GetDebugInfo();

	private:

		// Compute Device of abstract Operation
		const Device* m_device;

		MemoryManager m_memory_manager;

		// Computation queue of device
		cl::CommandQueue m_cl_queue;

		// Kernel and work group ranges
		cl::Kernel  m_cl_kernel;
		const std::string m_str_kernel_name;
		cl::NDRange m_ndrange_global;
		cl::NDRange m_ndrange_local;

		// Profiling
		cl::Event m_event;

		// Edge inputs and outputs
		std::vector<const Edge*> m_edge_inputs;
		std::vector<const Edge*> m_edge_outputs;
		std::map<size_t, const Edge*> m_map_arg_index_edge;

		// Native c/c++ kernel
		void (CL_CALLBACK *m_user_func)(void *);
		std::vector<void*>			 m_user_func_args;
		std::pair<void*, size_t>	 m_user_func_args_ptr;
		std::vector<cl::Memory>      m_user_func_mem_objects;
		std::vector<const void*>     m_user_func_mem_locs;
		std::map<size_t, cl::Memory> m_map_user_func_arg_mem_objects;
		std::map<size_t, void*>      m_map_user_func_arg_constants;
	
		// Synchronization
		SyncBarrier* m_sync_input;
		SyncBarrier* m_sync_output;

		void AddArgDebugStr( size_t index, const std::string& str);
		void BuildUserFuncArgs();

		void SyncInput();
		void SyncOutput();
		void Process();

		void PrintProfile();

		// Debug information
		std::map<size_t, std::string> m_map_arg_str;
		std::string                   m_string_debug;

		std::string NDRangeToString( const cl::NDRange& ndrange );
	};

} // End namespace Dataflow

#endif // OPERATION_HPP
