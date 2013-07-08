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

// OpenCL includes
#define __CL_ENABLE_EXCEPTIONS
#include "CL/cl.hpp"

// Project includes
#include "FrameworkTypes.hpp"
#include "Synchronization.hpp"
#include "MemoryManager.hpp"
#include "Device.hpp"
#include "Edge.hpp"
#include "Operation.hpp"

// TODO: you can actually retreive number of kernel aguments with a kernel info call
// make fixed size arrays to hold that info instead of maps. CL_KERNEL_NUM_ARGS

namespace Dataflow
{

	Operation::~Operation(){ }

	Operation::Operation( const Device* dev,
						  const std::string& str_kernel_name,
						  SyncBarrier* sb_in,
						  SyncBarrier* sb_out )
						  : m_device(dev),
							m_str_kernel_name(str_kernel_name),
							m_ndrange_global(cl::NDRange(0)),
							m_ndrange_local(cl::NullRange),
							m_user_func(nullptr),
							m_sync_input(sb_in),
							m_sync_output(sb_out)
	{
		cl_command_queue_properties prop = 0;
#ifdef PROFILE_OPERATION
		prop = CL_QUEUE_PROFILING_ENABLE;
#endif
		m_cl_queue  = cl::CommandQueue( dev->GetCLContext(), dev->GetCLDevice(), prop );
		m_cl_kernel = cl::Kernel( dev->GetCLProgram(), str_kernel_name.c_str() );
	}

	Operation::Operation( const Device* dev,
					      void (CL_CALLBACK *user_func)(void*),
					      SyncBarrier* sb_in,
					      SyncBarrier* sb_out )
					      :	m_device(dev),
					    	m_str_kernel_name("native c/c++ function"),
				            m_user_func(user_func),
					    	m_sync_input(sb_in),
					    	m_sync_output(sb_out)
	{
		cl_command_queue_properties prop = 0;
#ifdef PROFILE_OPERATION
		prop = CL_QUEUE_PROFILING_ENABLE;
#endif
		m_cl_queue  = cl::CommandQueue( dev->GetCLContext(), dev->GetCLDevice(), prop );
	}

	void Operation::BuildUserFuncArgs()
	{
		size_t num_mem  = m_map_user_func_arg_mem_objects.size();
		size_t num_args = m_map_user_func_arg_mem_objects.size() + m_map_user_func_arg_constants.size();

		m_user_func_args = std::vector<void*>(num_args);
		m_user_func_mem_objects = std::vector<cl::Memory>(num_mem);
		m_user_func_mem_locs = std::vector<const void*>(num_mem);

		for( size_t i = 0, m = 0; i < num_args; i++ )
		{
			//if( (arg_constant = m_map_user_func_arg_constants[i]) )
			if( m_map_user_func_arg_constants.find(i) != m_map_user_func_arg_constants.end() )
			{
				m_user_func_args[i] = m_map_user_func_arg_constants[i];
			}
			else
			{
				m_user_func_mem_objects[m] = m_map_user_func_arg_mem_objects[i];
				m_user_func_mem_locs[m] = &m_user_func_args[i];
				m++;
			}
		}
	    
		m_user_func_args_ptr.first = &m_user_func_args.front();
		m_user_func_args_ptr.second = sizeof(void*) * num_args;
	}

	void Operation::SetKernelArgMemory( size_t index,
			                            const Memory* dev_mem,
						                const Edge* edge )
	{
		std::stringstream ss;

		if( m_user_func )
		{
			m_map_user_func_arg_mem_objects[index] = m_device->GetCLMemory(dev_mem);
			BuildUserFuncArgs();
		}
		else
		{
			m_cl_kernel.setArg( index, m_device->GetCLMemory(dev_mem) );
		}

		// Keep track of which edge belongs to the argument memory transfer
		m_map_arg_index_edge[index] = edge;

		ss << "cl::Memory, val " << m_device->GetCLMemory(dev_mem)();

		AddArgDebugStr( index, ss.str() );
	}

	void Operation::SetKernelArgConstant( size_t index,
			                              size_t size,
								          const char* type,
								          const void* arg )
	{
		if( m_user_func )
		{
			// Create memory to keep a copy of the argument
			void* arg_ptr = m_memory_manager.NewRawMemory( size, arg );
			m_map_user_func_arg_constants[index] = arg_ptr;
			BuildUserFuncArgs();
		}
		else
		{
			m_cl_kernel.setArg( index, size, (void*)arg );
		}

		AddArgDebugStr( index, std::string(type) );
	}

	const Edge* Operation::GetEdgeAtArgIndex( size_t index )
	{
		return m_map_arg_index_edge[index];
	}

	SyncBarrier* Operation::GetInputSync()
	{
		return m_sync_input;
	}

	SyncBarrier* Operation::GetOutputSync()
	{
		return m_sync_output;
	}

	void Operation::SetInputSync( SyncBarrier* sb_in )
	{
		m_sync_input = sb_in;
	}

	void Operation::SetOutputSync( SyncBarrier* sb_out )
	{
		m_sync_output = sb_out;
	}

	void Operation::AddInputEdge( const Edge* edge )
	{
		m_edge_inputs.push_back(edge);
	}

	void Operation::AddOutputEdge( const Edge* edge )
	{
		m_edge_outputs.push_back(edge);
	}

	const Device* Operation::GetDevice() const
	{
		return m_device;
	}

	void Operation::SetWorkSize( const cl::NDRange& global,
								 const cl::NDRange& local )
	{
		m_ndrange_global = global;
		m_ndrange_local = local;
	}

	void Operation::SyncInput()
	{
		m_sync_input->Wait();
	}

	void Operation::SyncOutput()
	{
		m_sync_output->Wait();
	}

	void Operation::Process()
	{
		if( m_user_func )
		{
			m_cl_queue.enqueueNativeKernel( m_user_func,              /* Function pointer to call */
											m_user_func_args_ptr,     /* Function arguments */
											&m_user_func_mem_objects, /* Buffer objects to use */
											&m_user_func_mem_locs,    /* Buffer object data pointers to set in function arguments */
											NULL,                     /* Event wait list */
#ifdef PROFILE_OPERATION
											&m_event );               /* This event */
#else
											NULL );
#endif
		}
		else
		{
			m_cl_queue.enqueueNDRangeKernel( m_cl_kernel,       /* Kernel to execute */
											 cl::NullRange,     /* Offset range */
											 m_ndrange_global,  /* Global range */
											 m_ndrange_local,   /* Local range */
											NULL,               /* Event dependencies */
#ifdef PROFILE_OPERATION
											&m_event );         /* This event */
#else
											NULL );
#endif
		}

		m_cl_queue.finish();

#ifdef PROFILE_OPERATION
		PrintProfile();
#endif
	}

	void Operation::PrintProfile()
	{
		// Ticks of command duration
		unsigned long long ticks = ( m_event.getProfilingInfo<CL_PROFILING_COMMAND_END>()
						           - m_event.getProfilingInfo<CL_PROFILING_COMMAND_START>() );
	
		// Get interval in seconds
		double sec = ticks * 1e-9;

		// Ticks to start command
		ticks = ( m_event.getProfilingInfo<CL_PROFILING_COMMAND_START>()
				- m_event.getProfilingInfo<CL_PROFILING_COMMAND_QUEUED>() );
			
		// Get interval in microseconds
		double micro = ticks * 1e-3;

		std::cout << "Operation " << m_str_kernel_name << " exec/s " << sec << "\tcommand overhead usec: " << micro << std::endl;

		//Clear event?
		//m_event = cl::Event();
	}

	void Operation::ThreadStart( SyncStart* start, bool* execute )
	{
		start->Wait();

		while( *execute )
		{
			SyncInput();

			if( *execute )    // Value of execute changes to false upon termination
			{
				//Process();
			}

			SyncOutput();
		}
	}

	std::string Operation::NDRangeToString( const cl::NDRange& ndrange )
	{
		std::stringstream ss;

		ss << "[";

		switch( ndrange.dimensions() )
		{
			case 0 :
				ss << "0";
				break;
			case 1 :
				ss << ndrange[0];
				break;
			case 2:
				ss << ndrange[0] << "," << ndrange[1];
				break;
			case 3:
				ss << ndrange[0] << "," << ndrange[1] << "," << ndrange[2];
				break;
		}

		ss << "]";

		return ss.str();
	}

	void Operation::AddArgDebugStr( size_t index, const std::string& str )
	{
		m_map_arg_str[index] = str;
	}

	/* 
	  Get debug information of Operation, returns a string of
	  it's ID, queue, kernel information and events
	*/
	const std::string& Operation::GetDebugInfo()
	{
		std::stringstream ss;

		// Common APC info
		ss << "> Operation: " << this << " device: " << m_device->GetTypeStr()
		   << "(" << m_device->GetCLDeviceID() << ") computation queue: " << m_cl_queue() << std::endl;

		// Kernel info
		ss << "\t" << "Kernel: " << m_str_kernel_name;
	
		if( m_cl_kernel() )
		{
			ss << ", (" << m_cl_kernel() << "), global NDRange: " << NDRangeToString(m_ndrange_global) << ", local NDRange: "
			   << NDRangeToString(m_ndrange_local);
		}

		ss << std::endl;
	
		// Kernel arg info
		if( m_map_arg_str.size() != 0 )

		{
			ss << "\t" << "Kernel args: " << std::endl;

			for( std::pair<const size_t, std::string>& arg : m_map_arg_str )
			{
				ss << "\t\t" << arg.first << ": " << arg.second << std::endl;
			}
		}

		ss << "\t" << "Input sync " << m_sync_input->GetDebugInfo() << std::endl;
		ss << "\t" << "Output sync " << m_sync_output->GetDebugInfo() << std::endl;
	
		m_string_debug = ss.str();

		return m_string_debug;
	}

} // End namespace Dataflow
