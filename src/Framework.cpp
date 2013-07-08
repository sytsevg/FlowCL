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
#include <fstream>
#include <chrono>
#include <thread>
#include <atomic>

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
#include "Framework.hpp"

namespace Dataflow
{
	std::string OpenCLErrorCodeToString( cl::Error& err )
	{
		#define OPENCLERRORCODETOSTRING_CASE(X) case X : return #X;
		switch( err.err() ) 
		{
			OPENCLERRORCODETOSTRING_CASE(CL_SUCCESS                        )
			OPENCLERRORCODETOSTRING_CASE(CL_DEVICE_NOT_FOUND               )
			OPENCLERRORCODETOSTRING_CASE(CL_DEVICE_NOT_AVAILABLE           )
			OPENCLERRORCODETOSTRING_CASE(CL_COMPILER_NOT_AVAILABLE         )
			OPENCLERRORCODETOSTRING_CASE(CL_MEM_OBJECT_ALLOCATION_FAILURE  )
			OPENCLERRORCODETOSTRING_CASE(CL_OUT_OF_RESOURCES               )
			OPENCLERRORCODETOSTRING_CASE(CL_OUT_OF_HOST_MEMORY             )
			OPENCLERRORCODETOSTRING_CASE(CL_PROFILING_INFO_NOT_AVAILABLE   )
			OPENCLERRORCODETOSTRING_CASE(CL_MEM_COPY_OVERLAP               )
			OPENCLERRORCODETOSTRING_CASE(CL_IMAGE_FORMAT_MISMATCH          )
			OPENCLERRORCODETOSTRING_CASE(CL_IMAGE_FORMAT_NOT_SUPPORTED     )
			OPENCLERRORCODETOSTRING_CASE(CL_BUILD_PROGRAM_FAILURE          )
			OPENCLERRORCODETOSTRING_CASE(CL_MAP_FAILURE                    )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_VALUE                  )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_DEVICE_TYPE            )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_PLATFORM               )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_DEVICE                 )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_CONTEXT                )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_QUEUE_PROPERTIES       )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_COMMAND_QUEUE          )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_HOST_PTR               )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_MEM_OBJECT             )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR)
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_IMAGE_SIZE             )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_SAMPLER                )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_BINARY                 )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_BUILD_OPTIONS          )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_PROGRAM                )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_PROGRAM_EXECUTABLE     )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_KERNEL_NAME            )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_KERNEL_DEFINITION      )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_KERNEL                 )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_ARG_INDEX              )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_ARG_VALUE              )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_ARG_SIZE               )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_KERNEL_ARGS            )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_WORK_DIMENSION         )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_WORK_GROUP_SIZE        )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_WORK_ITEM_SIZE         )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_GLOBAL_OFFSET          )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_EVENT_WAIT_LIST        )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_EVENT                  )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_OPERATION              )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_GL_OBJECT              )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_BUFFER_SIZE            )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_MIP_LEVEL              )
			OPENCLERRORCODETOSTRING_CASE(CL_INVALID_GLOBAL_WORK_SIZE       )
		}
		#undef OPENCLERRORCODETOSTRING_CASE

		return "Unkown OpenCL Error";
	}

	/* Initialise framework defaults
	*/
	Framework::Framework()
	{
		m_threads_execute = false;
		m_sync_input = m_memory_manager.NewSyncBarrier()->Add();
		m_sync_output = m_memory_manager.NewSyncBarrier()->Add();
	}

	/* Joins running threads in framework
	*/
	void Framework::TerminateThreads()
	{   
		// Threads were previously running
		if( m_threads_execute )
		{
			// TODO Fix unelegant solution for resolving terminating deadlocks (only present on one test system)
			// Some threads are still blocked on output syncs, and when the m_threads_execute turns to false,
			// they terminate out of the loop, where they should still be doing a final synchronization
    		std::this_thread::sleep_for( std::chrono::milliseconds(1) );
			
			// Signal threads to exit loop
			m_threads_execute = false;
			// Release input threads
			SyncInput();
			// Wait for output threads
			SyncOutput();
		}
		// Threads are waiting on an initial start barrier
		else
		{
			// Release them
			m_sync_start.Start( m_threads.size() );
		}
	
		// Threads quit and host waits for them to terminate successfully
		for( std::thread& thread : m_threads )
		{
			thread.join();
		}
	}

	/* Termination
	*/
	Framework::~Framework() 
	{
		// pewpew
		TerminateThreads();
	}

	/* Check whether a file at a given path exists
	*/
	bool Framework::FileExists( const std::string& std_file_path )
	{
		std::ifstream ifs( std_file_path );
		return ifs.is_open();
	}

	/* Simple but effective djb2 hash for strings
	*/
	size_t HashString( const std::string& str )
	{
		unsigned long hash = 5381;
		char c;
		const char* c_ptr = str.c_str();
		
		while( c = *c_ptr++ )
		{
			hash = ((hash << 5) + hash) + c; // hash * 33 + c
		}

		return hash;
	}

	/* Generates the binary filename that will be used to retreive/create binary sources
	*/
	std::string Framework::GetBinaryFileName( unsigned long hash_kernel_source,
											  const cl::Device& cl_device )
	{
		std::string s = "openclkernelbin/" + std::to_string( (unsigned long long)hash_kernel_source ) 
			          + ' ' + cl_device.getInfo<CL_DEVICE_VENDOR>() + ' ' + cl_device.getInfo<CL_DEVICE_NAME>(); 
		return s;
	}

	/* Read a binary file at a given path and return file contents
	*/
	const void* Framework::ReadBinaryFile( const std::string& str_file_path, /* path of file */
		                                   size_t& out_size )                /* read number of bytes */
	{
		std::ifstream ifs( str_file_path, std::ifstream::binary | std::ifstream::ate);
		out_size = (size_t)ifs.tellg();
		char* bin_data = (char*)m_memory_manager.NewRawMemory( out_size );
		ifs.seekg(0, ifs.beg);
		ifs.read( bin_data, out_size );

		return bin_data;
	}

	/* Write a binary file time at a given path
	*/
	void Framework::WriteBinaryFile( const std::string& str_file_path, /* path of file */
		                             const char* bin,                  /* binary data */
									 size_t size )                     /* size of data */
	{
		std::ofstream ofs( str_file_path, std::ofstream::binary );
		ofs.write( bin, size );
	}

	/* Create and return program binaries from a given path
	*/
	cl::Program::Binaries Framework::CreateProgramBinariesFromFile( const std::string& str_binary_file_name )
	{
		size_t bin_size;
		const void* bin_data = ReadBinaryFile( str_binary_file_name, bin_size );
		std::vector<std::pair<const void*, size_t>> binaries;
		//binaries.push_back( std::pair<const void*, size_t>(bin_data, bin_size) );
		binaries.emplace_back( bin_data, bin_size );
		return cl::Program::Binaries( binaries );
	}

	/* Create a program for a given device
	   Function will check whether the program has been compiled before
	   Uses the compiled binaries for faster program creation
	*/
	cl::Program Framework::CreateProgramForDevice( const cl::Context& cl_context,
											       const std::string& str_kernel_source,
											       unsigned long hash_kernel_source,
											       const cl::Device& cl_device )
	{
		cl::Program program;
		std::string str_binary_file_name = GetBinaryFileName( hash_kernel_source, cl_device );

		// If there is a program binary available
		if( FileExists(str_binary_file_name) )
		{
			// Create a program with binaries
			try
			{
				cl::Program::Binaries cl_binaries = CreateProgramBinariesFromFile( str_binary_file_name );
				std::vector<cl::Device> cl_devices; cl_devices.emplace_back( cl_device );//cl_devices.push_back( cl_device );
				program = cl::Program( cl_context, cl_devices, cl_binaries );
				program.build();
			}
			catch( cl::Error err )
			{
				m_ss_builder << "Error using binary for program: " << err.what()
							 << "(...) returned " << OpenCLErrorCodeToString( err ) << "\n";
				m_string_error = m_ss_builder.str();
			
				throw std::runtime_error(m_string_error.c_str());
			}
		}
		// Compile kernel source
		else
		{
			try
			{
				program = cl::Program( cl_context, str_kernel_source );
				program.build();
				// Retreive binaries
				std::vector<size_t> size_binaries = program.getInfo<CL_PROGRAM_BINARY_SIZES>();
				std::vector<char*> binaries = program.getInfo<CL_PROGRAM_BINARIES>();
				// Store binary data into new file
				WriteBinaryFile( str_binary_file_name, binaries[0], size_binaries[0] );
			}
			catch( cl::Error err )
			{
				m_ss_builder << "Error building source for " << cl_device.getInfo<CL_DEVICE_VENDOR>() << "'s device "
							 << cl_device.getInfo<CL_DEVICE_NAME>() << ":" << std::endl << std::endl
							 << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>( cl_device ) << std::endl;//cluGetBuildErrors( program, cl_device ) + "\n";
				m_string_error = m_ss_builder.str();
			
				throw std::runtime_error(m_string_error.c_str());
			}
		}

		return program;
	}

	/* For each platform found on the system retrieve it's devices.
	   Create a context for each device and compile the given program.
	*/
	void Framework::CreateDevices( const std::string& str_kernel_source,
		                           unsigned long hash_kernel_source )
	{
		try
		{
			std::vector<cl::Platform> cl_platforms;
			cl::Platform::get( &cl_platforms );

			for( cl::Platform& cl_platform : cl_platforms )
			{
				std::vector<cl::Device> devices;

				cl_platform.getDevices( CL_DEVICE_TYPE_ALL, &devices );

				for( cl::Device& cl_device : devices )
				{
					cl::Context cl_context( cl_device );
					cl::Program cl_program = CreateProgramForDevice( cl_context, str_kernel_source, hash_kernel_source, cl_device );

					Device* dev = m_memory_manager.NewDevice( cl_device, cl_context, cl_program );

					m_devices.push_back( dev );
				}
			}
		}
		catch( cl::Error err )
		{
			m_ss_builder << "Error creating context for device: " << err.what()
			             << "(...) returned " << OpenCLErrorCodeToString( err ) << "\n";
			m_string_error = m_ss_builder.str();

			throw std::runtime_error( m_string_error.c_str() );
		}
	}

	const std::string& Framework::GetDeviceInfo()
	{
		try
		{
			cl::Error err = CL_SUCCESS;

			//return m_string_debug = std::string( cluGetDeviceInfo(err) );
			return m_string_debug;
		}
		catch( cl::Error err )
		{
			m_ss_builder << "Error retreiving device information: " << err.what()
			             << "(...) returned " << OpenCLErrorCodeToString( err ) << "\n";
			m_string_error = m_ss_builder.str();

			throw std::runtime_error( m_string_error.c_str() );
		}
	}

	/* Compile a given file path and initialize all available Devices */
	void Framework::CompileFile( const std::string& str_file_path )
	{
		if( FileExists(str_file_path) )
		{
			std::ifstream ifs(str_file_path);
			std::string str_file_contents( (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>() );
			unsigned long hash_file_contents = HashString( str_file_contents );

			CreateDevices( str_file_contents, hash_file_contents );
		}
		else
		{
			m_string_error = "File \"" + str_file_path + "\" not found.\n";

			throw std::runtime_error( m_string_error.c_str() );
		}
	}

	/* Compile a given source and initialize all available devices */
	void Framework::CompileSource( const std::string& str_source )
	{
		unsigned long hash_file_contents = HashString( str_source );

		CreateDevices( str_source, hash_file_contents );
	}

	/* Returns the available devices that can be used by the framework.	*/
	const std::vector<Device*>& Framework::GetDevices()
	{
		return m_devices;
	}

	/* Iterate through all Compute devices and retrieve a device with given type
	*/
	const Device* Framework::GetDeviceByCLType( cl_device_type type ) const
	{
		for( Device* dev : m_devices )
		{
			if( dev->GetCLDeviceType() == type )
			{
				return dev;
			}
		}

		return nullptr;
	}

	/* Return first occurence of a CPU Device */
	const Device* Framework::GetCPUDevice() const
	{
		return GetDeviceByCLType( CL_DEVICE_TYPE_CPU );
	}

	/* Return first occurence of a GPU Device */
	const Device* Framework::GetGPUDevice() const
	{
		return GetDeviceByCLType( CL_DEVICE_TYPE_GPU );
	}

	/* Checkes whether a queue to create pinned memory has already been created */
	bool Framework::PinnedQueueCreated() const
	{
		return m_pinned_cl_queue() != nullptr;
	}

	/* To create pinned memory, some key concepts must be realised. 
	   Every device has it's own context, a buffer must be made for every device, 
	   representing one shared buffer between all devices.

	   The ideal scenario is to have one GPU device. This one will be selected to create pinned memory
	   for fastest interconnect between host -> GPU

	   1) Select Compute Device that is a GPU
	   2) If the Compute Device is not a GPU
		  a) Select first Compute Device
	   3) Create pinned memory
	   4) Store pinned memory
	   5) For all devices that are not GPU's, create default buffer
		  if device is CPU, create buffer with CL_MEM_USE_HOST_PTR of pinned memory
	*/
	const Memory* Framework::CreatePinnedMemory( size_t size )
	{
		try
		{
			const Device* dev = GetGPUDevice();
			void* ptr_mem;

			if( !dev )
			{
				dev = m_devices[0];
			}

			if( !PinnedQueueCreated() )
			{
				m_pinned_cl_queue = cl::CommandQueue( dev->GetCLContext(), dev->GetCLDevice() );
			}

			// Create page locked memory on host for maximum interconnect bandwidth transfers
			cl::Buffer buf( dev->GetCLContext(), CL_MEM_ALLOC_HOST_PTR, size );

			// Map the region of the page locked area and retrieve a pointer to that memory
			ptr_mem = m_pinned_cl_queue.enqueueMapBuffer( buf, CL_TRUE, CL_MAP_READ, 0, size );

			// Create device memory identifier
			const Memory* dev_mem = m_memory_manager.NewMemory( size, ptr_mem, m_device_memories.size() );

			// Retain memory object or it will be deleted
			//m_pinned_cl_memories.push_back( buf );
			m_pinned_cl_memories.emplace_back( buf );

			for( Device* dev : m_devices )
			{
				if( dev->IsDedicatedDevice() )
				{
					dev->AddMemoryObject( cl::Buffer(dev->GetCLContext(), (cl_mem_flags)NULL, size) );
				}
				else if( dev->IsCPU() )
				{
					dev->AddMemoryObject( cl::Buffer(dev->GetCLContext(), CL_MEM_USE_HOST_PTR, size, dev_mem->ptr_mem) );
				}
			}

			// Copy newly created device memory object
			m_device_memories.push_back( dev_mem );

			return dev_mem;
		}
		catch( cl::Error err )
		{
			m_ss_builder << "Error creating memory: " << err.what()
			             << "(...) returned " << OpenCLErrorCodeToString( err ) << "\n";
			m_string_error = m_ss_builder.str();

			throw std::runtime_error( m_string_error.c_str() );
		}
	}

	/* Create high performance pinned memory of a given size */
	const Memory* Framework::CreateMemory( size_t size )
	{
		return CreatePinnedMemory( size );
	}

	/* Create a new Operation that executes the given function name on a given Device */
	Operation* Framework::CreateDeviceOperation( const Device* dev,                 /* Pointer to a valid device which Operation will use */ 
										         const std::string& str_func_name ) /* Kernel to use */
	{
		try
		{
			// Create new Operation object
			Operation* new_apc = m_memory_manager.NewOperation( dev,                    /* Device to use */
																str_func_name,          /* Kernel name to run */
																m_sync_input->Add(),    /* Input synchronization */
																m_sync_output->Add() ); /* Output synchronization */

			//m_threads.push_back( std::thread(&Operation::ThreadStart, new_apc, &m_sync_start, &m_threads_execute) );
			m_threads.emplace_back( &Operation::ThreadStart, new_apc, &m_sync_start, &m_threads_execute );

			// Store object in list
			m_operations.push_back( new_apc );

			return new_apc;
		}
		catch( cl::Error err )
		{
			m_ss_builder << "Error creating Operation: " << err.what()
			             << "(...) returned " << OpenCLErrorCodeToString( err ) << "\n";
			m_string_error = m_ss_builder.str();

			throw std::runtime_error( m_string_error.c_str() );
		}
	}

	/* Create a new Operation that executes a given native C/C++ user function */
	Operation* Framework::CreateNativeOperation( const Device* dev,               /* Pointer to a valid device which Operation will use */
										         void (CL_CALLBACK *user_func)(void*) ) /* User Function */
	{
		if( dev->CanExecuteNativeKernel() )
		{
			// Create new Operation object
			Operation* new_apc = m_memory_manager.NewOperation( dev,                    /* Device to use */
																user_func,		        /* User function to call */
																m_sync_input->Add(),	/* Input synchronization */
																m_sync_output->Add() ); /* Output synchronization */

			//m_threads.push_back( std::thread(&Operation::ThreadStart, new_apc, &m_sync_start, &m_threads_execute) );
			m_threads.emplace_back( &Operation::ThreadStart, new_apc, &m_sync_start, &m_threads_execute );

			// Store object in list
			m_operations.push_back( new_apc );

			return new_apc;
		}
		else
		{
			m_ss_builder << "Device " << dev->GetNameStr() << " cannot execute native kernels\n";
			m_string_error = m_ss_builder.str();

			throw std::runtime_error( m_string_error.c_str() );
		}
	}

	Edge* Framework::CreateInputEdge( Operation* op_in,
			                          const Memory* dev_mem,
							          EdgeType type,
							          CopyInterval copy_interval  )
	{
		Edge* edge = m_memory_manager.NewEdge( nullptr,         /* input source*/
											   op_in,           /* output destination */
											   dev_mem,         /* Device memory */
											   type,            /* Edge type */
											   copy_interval ); /* Copy Interval */

		edge->SetInputSync( m_sync_input->Add() );

		if( op_in->GetInputSync() == m_sync_input )
		{
			m_sync_input->Remove();
			SyncBarrier* sb = m_memory_manager.NewSyncBarrier();
			edge->SetOutputSync( sb->Add() );
			op_in->SetInputSync( sb->Add() );
		}
		else
		{
			edge->SetOutputSync( op_in->GetInputSync()->Add() );
		}

		//m_threads.push_back( std::thread(&Edge::ThreadStart, edge, &m_sync_start, &m_threads_execute) );
		m_threads.emplace_back( &Edge::ThreadStart, edge, &m_sync_start, &m_threads_execute );

		op_in->AddInputEdge( edge );

		return edge;
	}

	Edge* Framework::CreateOutputEdge( Operation* op_out,
			                           const Memory* dev_mem,
								       EdgeType type,
								       CopyInterval copy_interval )
	{
		Edge* edge = m_memory_manager.NewEdge( op_out,          /* input source */
											   nullptr,         /* output destination */
											   dev_mem,         /* Device memory */
											   type,            /* Edge type */
											   copy_interval ); /* Copy Interval */

		edge->SetOutputSync( m_sync_output->Add() );

		if( op_out->GetOutputSync() == m_sync_output )
		{
			m_sync_output->Remove();
			SyncBarrier* sb = m_memory_manager.NewSyncBarrier();
			op_out->SetOutputSync( sb->Add() );
			edge->SetInputSync( sb->Add() );
		}
		else
		{
			edge->SetInputSync( op_out->GetOutputSync()->Add() );
		}

		//m_threads.push_back( std::thread(&Edge::ThreadStart, edge, &m_sync_start, &m_threads_execute) );
		m_threads.emplace_back( &Edge::ThreadStart, edge, &m_sync_start, &m_threads_execute );

		op_out->AddOutputEdge( edge );

		return edge;
	}

	Edge* Framework::CreateOpToOpEdge( Operation* op_in,
			                           Operation* op_out,
								       const Memory* dev_mem,
								       EdgeType type,
								       CopyInterval copy_interval )
	{
		Edge* edge = m_memory_manager.NewEdge( op_in,           /* input source */
											   op_out,          /* output destination */
											   dev_mem,         /* memory */
											   type,            /* Edge type */
		                                       copy_interval ); /* Copy interval */

		if( op_in->GetOutputSync() == m_sync_output )
		{
			m_sync_output->Remove();
			SyncBarrier* sb = m_memory_manager.NewSyncBarrier();
			op_in->SetOutputSync( sb->Add() );
			edge->SetInputSync( sb->Add() );
		}
		else
		{
			edge->SetInputSync( op_in->GetOutputSync()->Add() );
		}

		if( op_out->GetInputSync() == m_sync_input )
		{
			m_sync_input->Remove();
			SyncBarrier* sb = m_memory_manager.NewSyncBarrier();
			op_out->SetInputSync( sb->Add() );
			edge->SetOutputSync( sb->Add() );
		}
		else
		{
			edge->SetOutputSync( op_out->GetInputSync()->Add() );
		}

		//m_threads.push_back( std::thread(&Edge::ThreadStart, edge, &m_sync_start, &m_threads_execute) );
		m_threads.emplace_back( &Edge::ThreadStart, edge, &m_sync_start, &m_threads_execute );

		op_in->AddOutputEdge( edge );
		op_out->AddInputEdge( edge );

		return edge;
	}

	/* Set an argument with memory that will not be transferred to the Operation device */
	void Framework::SetOperationArgMemory( Operation* op,      /* Pointer to a valid Operation to set argument */
			                               size_t index,       /* Index of the argument */
						                   const Memory* arg ) /* Argument memory */
	{
		try
		{
			// Set kernel argument 
			op->SetKernelArgMemory( index, arg, nullptr );
		}
		catch( cl::Error err )
		{
			m_ss_builder << "Error setting Operation argument at index " << index << ": " << err.what()
			             << "(...) returned " << OpenCLErrorCodeToString( err ) << "\n";
			m_string_error = m_ss_builder.str();

			throw std::runtime_error( m_string_error.c_str() );
		}
	}

	/* Set an argument with memory that will remain constant on the Operation device */
	void Framework::SetOperationArgConstant( Operation* op,    /* Pointer to a valid Operation to set local argument */
							                 size_t index,     /* Index of the argument */
							                 size_t size,      /* Size of the argument data */
							                 const char* type, /* Char identifier of the argument eg "int", "float" */
							                 const void* arg ) /* Pointer to argument data */
	{
		try
		{
			// Set kernel argument 
			op->SetKernelArgConstant( index, size, type, arg );
		}
		catch( cl::Error err )
		{
			m_ss_builder << "Error setting Operation argument at index " << index << ": " << err.what()
			             << "(...) returned " << OpenCLErrorCodeToString( err ) << "\n";
			m_string_error = m_ss_builder.str();

			throw std::runtime_error( m_string_error.c_str() );
		}
	}

	/* Set an argument with memory that will write from host memory to Operation device memory */
	void Framework::SetOperationArgInput( Operation* op,               /* Pointer to a valid Operation to set argument */
							              size_t index,                /* Index of the argument */
							              const Memory* arg,           /* Argument */
							              CopyInterval copy_interval ) /* Argument copy interval */
	{
		try
		{
			Edge* edge;
		
			// Transfers from host to CPU is a No Op
			if( op->GetDevice()->IsCPU() )
			{
				edge = CreateInputEdge( op, arg, EdgeType::NoOp, copy_interval );
			}
			// Explicit Host to Device memory tansfer required
			else
			{
				edge = CreateInputEdge( op, arg, EdgeType::HostToDevice, copy_interval );
			}

			// Keep track of edge
			m_edges.push_back( edge );

			// Set kernel argument 
			op->SetKernelArgMemory( index, arg, edge );
		}
		catch( cl::Error err )
		{
			m_ss_builder << "Error setting Operation input argument argument at index " << index << ": " << err.what()
			             << "(...) returned " << OpenCLErrorCodeToString( err ) << "\n";
			m_string_error = m_ss_builder.str();

			throw std::runtime_error( m_string_error.c_str() );
		}
	}

	/* Set an argument with memory that will write from Operation device memory to host memory */
	void Framework::SetOperationArgOutput( Operation* op,               /* Pointer to a valid Operation to set argument */
							               size_t index,                /* Index of the argument */
							               const Memory* arg,           /* Argument */
							               CopyInterval copy_interval ) /* Argument copy interval */
	{
		try
		{
			Edge* edge;
		
			// Transfers from CPU to host is a No Op
			if( op->GetDevice()->IsCPU() )
			{
				edge = CreateOutputEdge( op, arg, EdgeType::NoOp, copy_interval );
			}
			// Explicit device memory to host tansfer required
			else
			{
				edge = CreateOutputEdge( op, arg, EdgeType::DeviceToHost, copy_interval );
			}

			// Keep track of edge
			m_edges.push_back( edge );

			// Set kernel argument 
			op->SetKernelArgMemory( index, arg, edge );
		}
		catch( cl::Error err )
		{
			m_ss_builder << "Error setting Operation output argument at index " << index << ": " << err.what()
			             << "(...) returned " << OpenCLErrorCodeToString( err ) << "\n";
			m_string_error = m_ss_builder.str();

			throw std::runtime_error( m_string_error.c_str() );
		}
	}

	/* Set an argument with memory that will be read from Operation dependency device memory to Operation device memory */
	void Framework::SetOperationArgDependency( Operation* op,               /* Pointer to a valid Operation to set argument */
									           Operation* dependency,       /* Pointer to a valid Operation to set dependency */
									           size_t index,                /* Index of the argument */
									           const Memory* arg,           /* Memory to be used in argument */
									           CopyInterval copy_interval ) /* Argument copy interval */
	{
		try
		{
			Edge* edge;

			// Devices are the same; NoOp
			if( dependency->GetDevice() == op->GetDevice() )
			{
				edge = CreateOpToOpEdge( dependency, op, arg, EdgeType::NoOp, copy_interval  );
			}
			// CPU to Device
			else if( dependency->GetDevice()->IsCPU() && op->GetDevice()->IsDedicatedDevice() )
			{
				edge = CreateOpToOpEdge( dependency, op, arg, EdgeType::HostToDevice, copy_interval  );
			}
			// Device to CPU
			else if( dependency->GetDevice()->IsDedicatedDevice() && op->GetDevice()->IsCPU() )
			{
				edge = CreateOpToOpEdge( dependency, op, arg, EdgeType::DeviceToHost, copy_interval  );
			}
			// Devices are different; both are dedicated
			else if( dependency->GetDevice()->IsDedicatedDevice() && op->GetDevice()->IsDedicatedDevice() )
			{
				throw std::runtime_error( "Two different dedicated devices memory transfer not implemented\n" );
			}
			
			// Keep track of created edge
			m_edges.push_back( edge );

			op->SetKernelArgMemory( index, arg, edge );
		}
		catch( cl::Error err )
		{
			m_ss_builder << "Error setting Operation dependency: argument at index " << index << ": " << err.what()
			             << "(...) returned " << OpenCLErrorCodeToString( err ) << "\n";
			m_string_error = m_ss_builder.str();

			throw std::runtime_error( m_string_error.c_str() );
		}
	}

	/* Set the work size (granularity) of an Operation */
	void Framework::SetOperationWorkSize( Operation* op,             /* Pointer to a valid Operation to set work size */
										  const cl::NDRange& global, /* Global work size */
										  const cl::NDRange& local ) /* Local work size */
	{
		op->SetWorkSize( global, local );
	}

	/* Set the copy interval of an argument memory input/output defined at index */
	void Framework::SetOperationArgMemoryCopyInterval( Operation* op,               /* Pointer to a valdid Operation to set copy interval */
			                                           size_t index,				/* Index of argument to which the copy interval applies */
										               CopyInterval copy_interval )	/* Copy interval */
	{
		Edge* edge;

		if( (edge = (Edge*)op->GetEdgeAtArgIndex(index)) )
		{
			edge->SetCopyInterval( copy_interval );
		}
		else
		{
			m_ss_builder << "Error setting MemoryCopyInterval at Operation: argument index " << index << " not found.\n";
			m_string_debug = m_ss_builder.str();

			throw std::runtime_error( m_string_debug.c_str() );
		} 
	}

	void Framework::SyncInput()
	{
		m_sync_input->Wait();
	}

	void Framework::SyncOutput()
	{
		m_sync_output->Wait();
	}

	void Framework::Run()
	{
		if( !m_threads_execute )
		{
			m_threads_execute = true;
			m_sync_start.Start( m_threads.size() );
		}

		#ifdef PROFILE_FRAMEWORK
		std::chrono::system_clock::time_point start = std::chrono::steady_clock::now();
		#endif

		// Release input threads
		SyncInput();
		// Wait for output threads
		SyncOutput();

		#ifdef PROFILE_FRAMEWORK
		std::chrono::system_clock::time_point stop = std::chrono::steady_clock::now();
		double t = 1e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start).count();
		std::cout << "Framework run interval seconds: " << t << std::endl << std::endl;
		#endif
	}

	const std::string& Framework::GetDebugInfo()
	{     
		std::stringstream ss;

		ss << "Debug information from edges and operations" << std::endl << std::endl;

		for( Operation* op : m_operations )
		{
			ss << op->GetDebugInfo() << std::endl;
		}

		for( Edge* edge : m_edges )
		{
			ss << edge->GetDebugInfo() << std::endl;
		}

		ss << "> FlowCL thread count: " << m_threads.size() 
		   << ", Operations: " << m_operations.size() << ", Edges: " << m_edges.size() << std::endl;

		ss << "> Input sync: " << m_sync_input->GetDebugInfo() << " output sync: " << m_sync_output->GetDebugInfo() << std::endl;

		m_string_debug = ss.str();

		return m_string_debug;
	}

} // End namespace Dataflow
