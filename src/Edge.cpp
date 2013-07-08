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

// STD includes
#include <math.h>

// OpenCL includes
#define __CL_ENABLE_EXCEPTIONS
#include "CL/cl.hpp"

// Project includes
#include "FrameworkTypes.hpp"
#include "Synchronization.hpp"
#include "MemoryManager.hpp"
#include "Operation.hpp"
#include "Device.hpp"
#include "Edge.hpp"

namespace Dataflow
{

	// Edge Constructor
	Edge::Edge( const Operation* op_input,
				const Operation* op_output,
				const Memory* dev_mem,
				const EdgeType type,
				CopyInterval copy_interval )
				: m_op_input(op_input),
				  m_op_output(op_output),
                  m_dev_mem(dev_mem),
				  m_dev_input(op_input ? op_input->GetDevice() : nullptr),
				  m_dev_output(op_output ? op_output->GetDevice() : nullptr),
				  m_type(type),
				  m_copy_interval(copy_interval)
	{
		cl_command_queue_properties prop = 0;
#ifdef PROFILE_EDGE
		prop = CL_QUEUE_PROFILING_ENABLE;
#endif
		if( m_type == HostToDevice )
		{
			m_cl_queue_output = cl::CommandQueue( m_dev_output->GetCLContext(), m_dev_output->GetCLDevice(), prop );
		}
		else if( m_type == DeviceToHost )
		{
			m_cl_queue_input = cl::CommandQueue( m_dev_input->GetCLContext(), m_dev_input->GetCLDevice(), prop );
		}
		else if( m_type == DeviceToDevice )
		{
			throw std::runtime_error( "Two different devices memory transfer not implemented" );
		}
	}

	Edge::~Edge() { }

	void Edge::SetInputSync( SyncBarrier* in )
	{
		m_sync_input = in;
	}

	void Edge::SetOutputSync( SyncBarrier* out )
	{
		m_sync_output = out;
	}

	const Memory* Edge::GetMemory()
	{
		return m_dev_mem;
	}

	void Edge::SetCopyInterval( CopyInterval copy_interval )
	{
		m_copy_interval = copy_interval;
	}

	CopyInterval Edge::GetCopyInterval()
	{
		return m_copy_interval;
	}

	/* Handles memory that is of type HostInputMemory
	*/
	void Edge::HandleHostToDeviceTransfer()
	{
		// Explicitly write data from host allocated memory into device
		// Output memory to write to is m_dev_output
		m_cl_queue_output.enqueueWriteBuffer( m_dev_output->GetCLMemory( m_dev_mem ), /* Buffer to be written to */
											  CL_TRUE,                                /* Blocking */
											  0,                                      /* Offset */
											  m_dev_mem->size,                        /* Bytes to copy */
											  m_dev_mem->ptr_mem,                     /* Source */
											  NULL,                                   /* Dependent events */
#ifdef PROFILE_EDGE
											  &m_event );                             /* Event of write buffer */
#else
											  NULL );                                 /* Event of write buffer */
#endif
	}

	void Edge::HandleDeviceToHostTransfer()
	{
		// Read data from device memory into host allocated memory
		// Input memory to read from is m_dev_input
		m_cl_queue_input.enqueueReadBuffer( m_dev_input->GetCLMemory( m_dev_mem ), /* Buffer to read from */
											CL_TRUE,                               /* Blocking */
											0,                                     /* Offset */
											m_dev_mem->size,                       /* Bytes to copy */
											m_dev_mem->ptr_mem,                    /* Destination */
											NULL,                                  /* Dependent Events */
#ifdef PROFILE_EDGE
											&m_event );                            /* Event of write buffer */
#else
											NULL );                                /* Event of write buffer */
#endif
	}

	void Edge::HandleDeviceToDeviceTransfer()
	{
		throw std::runtime_error("Two different devices memory transfer not implemented");
	}

	void Edge::SyncInput()
	{
		m_sync_input->Wait();
	}

	void Edge::SyncOutput()
	{
		m_sync_output->Wait();
	}

	void Edge::TransferMemory()
	{	
		if( m_copy_interval != Never )
		{
			if( m_type == HostToDevice )
			{
				HandleHostToDeviceTransfer();
			}
			else if( m_type == DeviceToHost )
			{
				HandleDeviceToHostTransfer();
			}
			else if( m_type == DeviceToDevice )
			{
				HandleDeviceToDeviceTransfer();
			}

			m_copy_interval = m_copy_interval == Once ? Never : m_copy_interval;
		}
#ifdef PROFILE_EDGE
		PrintProfile();
#endif
	}

	void Edge::ThreadStart( SyncStart* start, bool* execute )
	{
		start->Wait();
	
		while( *execute )
		{
			SyncInput();

			if( *execute )           // Value of execute changes to false upon termination
			{
				//TransferMemory();
			}

			SyncOutput();
		}
	}

	void Edge::PrintProfile()
	{
		if( m_type != EdgeType::NoOp )
		{
			// Interval resolution ... apparently it always returns nanoseconds?
			int res = 1;
		
			if( m_type == HostToDevice )
			{
				//res = m_dev_output->GetCLDevice().getInfo<CL_DEVICE_PROFILING_TIMER_RESOLUTION>();
				std::cout << "Edge      " << "Host to " << m_dev_output->GetNameStr();
			}
			else if( m_type == DeviceToHost )
			{
				//res = m_dev_input->GetCLDevice().getInfo<CL_DEVICE_PROFILING_TIMER_RESOLUTION>();
				std::cout << "Edge      " << m_dev_input->GetNameStr() << " to Host";
			}

			// Ticks of command duration
			unsigned long long ticks = ( m_event.getProfilingInfo<CL_PROFILING_COMMAND_END>()
							           - m_event.getProfilingInfo<CL_PROFILING_COMMAND_START>() ) 
							           * res;
			// Get interval in seconds
			double sec = ticks * 1e-9;

			// Calculate bandwidth in MB/s
			double bandwidth = (m_dev_mem->size / sec) / pow(2.0, 20.0);
			bandwidth = floor(bandwidth * 100000) / 100000.0;                // round down to 6 precision

			// Ticks to start command
			ticks = ( m_event.getProfilingInfo<CL_PROFILING_COMMAND_START>()
				    - m_event.getProfilingInfo<CL_PROFILING_COMMAND_QUEUED>() ) 
				    * res;
			
			// Get interval in microseconds
			double micro = ticks * 1e-3;

			std::cout << " MB/s:  " << bandwidth << "\t\tcommand overhead usec: " << micro << std::endl;
		}
		else
		{
			std::cout << "Edge      " << m_dev_input << " zerocopy" << std::endl;
		}
	}

	const std::string& Edge::GetDebugInfo()
	{
		std::stringstream ss;

		ss.clear();

		// Common Edge info
		ss << "> Edge " << EdgeTypeString[m_type] << ": " << this << " copy interval: " << CopyIntervalString[m_copy_interval];
	
		// Device and queue info
		if( m_op_input ) 
		{
			ss << " input device: " <<  m_dev_input->GetTypeStr() << "(" << m_dev_input->GetCLDeviceID() << ")";
		}
		if( m_op_output )
		{
			ss << " output device: " << m_dev_output->GetTypeStr() << "(" << m_dev_output->GetCLDeviceID() << ")";
		}

		ss << std::endl;

		// Connected Operation info
		if( m_op_input )
		{
			ss << "\t" << "Operation: " << m_op_input << std::endl;
		}
		else
		{
			ss << "\t" << "Host" << std::endl;
		}
		ss << "\t" << "  |" << std::endl;
		ss << "\t" << "  v" << std::endl;
		if( m_op_output )
		{
			ss << "\t" << "Operation: " << m_op_output << std::endl;
		}
		else
		{
			ss << "\t" << "Host" << std::endl;
		}

		// Device memory its transferring
		ss << "\t" << "Device memory size: " << m_dev_mem->size << ", ptr_mem: " << m_dev_mem->ptr_mem << std::endl;

		if( m_op_input ) 
		{
			ss << "\t" << "\t" << "Input device memory object: " << m_dev_input->GetCLMemory( m_dev_mem )() << std::endl;
		}
		if( m_op_output )
		{
			ss << "\t" << "\t" << "Output device memory object: " << m_dev_output->GetCLMemory( m_dev_mem )() << std::endl;
		}

		ss << "\t" << "Input sync: " << m_sync_input->GetDebugInfo() << std::endl;
		ss << "\t" << "Output sync: " << m_sync_output->GetDebugInfo() << std::endl;

		m_string_debug = ss.str();

		return m_string_debug;
	}

} // End namespace Dataflow
