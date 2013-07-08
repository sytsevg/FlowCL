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
 Processing Framework

 The framework provides API to create a device data flow processing framework
 with abstract processing components. These components represent operations
 connected with input dependencies and outputs.

 Author: S.D.M. van Geldermalsen

 Date: 14-09-2012
*/

#ifndef FRAMEWORK_HPP
#define FRAMEWORK_HPP

// more error checking

namespace Dataflow
{

	class Framework
	{

	public:

		Framework();
	   ~Framework();

	    /* Compile a given file path and initialize all available Devices */
	    void CompileFile( const std::string& str_file_path );

		/* Compile a given source and initialize all available devices */
		void CompileSource( const std::string& str_source );

		/* Returns the available devices that can be used by the framework.	*/
		const std::vector<Device*>& GetDevices(); 

		/* Create high performance pinned memory of a given size */
		const Memory* CreateMemory( size_t size );

		/* Return first occurence of a certain Device */
		const Device* GetCPUDevice() const;
		const Device* GetGPUDevice() const;

		/* Create a new Operation that executes the given function name on a given Device */
		Operation* CreateDeviceOperation( const Device* dev,                  /* Pointer to a valid device which Operation will use */
									      const std::string& str_func_name ); /* Kernel to use */

		/* Create a new Operation that executes a given native C/C++ user function */
		Operation* CreateNativeOperation( const Device* dev,                      /* Pointer to a valid device which Operation will use */
									      void (CL_CALLBACK *user_func)(void*) ); /* User Function */

		/* Set an argument with memory that will not be transferred to the Operation's Device */
		void SetOperationArgMemory( Operation* op,       /* Pointer to a valid Operation to set argument */
			                        size_t index,        /* Index of the argument */
							        const Memory* arg ); /* Argument memory */

		/* Set an argument with memory that will remain constant on the Operation device */
		void SetOperationArgConstant( Operation* op,     /* Pointer to a valid Operation to set local argument */
							          size_t index,      /* Index of the argument */
							          size_t size,       /* Size of the argument data */
							          const char* type,  /* Char identifier of the argument eg "int", "float" */
							          const void* arg ); /* Pointer to argument data */

		/* Set an argument with memory that will be written from host memory to Operation device memory */
		void SetOperationArgInput( Operation* op,                /* Pointer to a valid Operation to set argument */
							       size_t index,                 /* Index of the argument */
							       const Memory* arg,            /* Argument */
							       CopyInterval copy_interval ); /* Argument copy interval */

		/* Set an argument with memory that will be written from Operation device memory to host memory */
		void SetOperationArgOutput( Operation* op,                /* Pointer to a valid Operation to set argument */
							        size_t index,                 /* Index of the argument */
							        const Memory* arg,            /* Argument */
							        CopyInterval copy_interval ); /* Argument copy interval */

		/* Set an argument with memory that will be read from Operation dependency device memory to Operation device memory */
		void SetOperationArgDependency( Operation* op,                /* Pointer to a valid Operation to set argument */
									    Operation* dependency,        /* Pointer to a valid Operation to set dependency */
									    size_t index,                 /* Index of the argument */
									    const Memory* dev_mem,        /* Memory to be used in argument */
									    CopyInterval copy_interval ); /* Argument copy interval */

		/* Set the work size (granularity) of an Operation */
		void SetOperationWorkSize( Operation* op,              /* Pointer to a valid Operation to set work size */
								   const cl::NDRange& global,  /* Global work size */
								   const cl::NDRange& local ); /* Local work size */

		/* Set the copy interval of an argument memory input/output defined at index */
		void SetOperationArgMemoryCopyInterval( Operation* op,                /* Pointer to a valdid Operation to set copy interval */
			                                    size_t index,                 /* Index of argument to which the copy interval applies */
												CopyInterval copy_interval ); /* Copy interval */

		/* Blocking run */
		void Run();

		/* Retrieve detailed Device info */
		const std::string& GetDeviceInfo();

		/* Retrieve detailed debug info of created Operations and Edges */
		const std::string& GetDebugInfo();
    
	private:

    	/* Memory manager to handle new/delete */
		MemoryManager m_memory_manager;

		/* Available devices on host */
		std::vector<Device*>       m_devices;
		/* Operations */	    
		std::vector<Operation*>    m_operations;
		/* Edges that connect Operations */
		std::vector<Edge*>         m_edges;
		/* Memory descriptor for memory on devices */
		std::vector<const Memory*> m_device_memories;
		/* Pinned memory */
		std::vector<cl::Memory>    m_pinned_cl_memories;
		/* Queue to create pinned memory */
		cl::CommandQueue           m_pinned_cl_queue;
		/* Debug string of processing components and edges */
		std::string                m_string_debug;
		/* Error string building */
		std::string                m_string_error;
		std::stringstream          m_ss_builder;

		/* Threading objects for processing components and edges */
		std::vector<std::thread> m_threads;
		bool                     m_threads_execute;

		/* Synchronization */
		SyncStart    m_sync_start;
		SyncBarrier* m_sync_input;
		SyncBarrier* m_sync_output;

		/* Built binaries */
		bool FileExists( const std::string& str_file_path );

		/* Generates the binary filename that will be used to retreive/create binary sources */
		std::string GetBinaryFileName( unsigned long hash_kernel_source,
									   const cl::Device& cl_device );

		/* Read a binary file at a given path and return file contents	*/
		const void* ReadBinaryFile( const std::string& str_file_path, size_t& out_bytes_read );

		/* Write a binary file time at a given path	*/
		void WriteBinaryFile( const std::string& str_file_path, const char* bin, size_t size );

		/* Create and return program binaries from a given path	*/
		cl::Program::Binaries CreateProgramBinariesFromFile( const std::string& str_file_path );

		/* Create a program for a given device with context, kernel source */
		cl::Program CreateProgramForDevice( const cl::Context& cl_context,
								            const std::string& str_kernel_source,
								            unsigned long hash_kernel_source,
								            const cl::Device& cl_device );

		/* Create Devices for all available cl::Devices on host */
		void CreateDevices( const std::string& str_kernel_source,
			                unsigned long hash_kernel_source );

		/* Return Device that matches a cl_device_type */
		const Device* GetDeviceByCLType( cl_device_type type ) const;

		/* Checkes whether a queue to create pinned memory has already been created */
		bool PinnedQueueCreated() const;

		/* Create pinned memory of given size */
		const Memory* CreatePinnedMemory( size_t size );

		/* Edge creation */
		Edge* CreateInputEdge( Operation* op_in,
			                   const Memory* dev_mem,
							   EdgeType type,
							   CopyInterval copy_interval );

		Edge* CreateOutputEdge( Operation* op_out,
			                    const Memory* dev_mem,
								EdgeType type,
								CopyInterval copy_interval );

		Edge* CreateOpToOpEdge( Operation* op_in,
			                    Operation* op_out,
								const Memory* dev_mem,
								EdgeType type,
								CopyInterval copy_interval );

		/* Threading functions */
		void CreateThreads();
		void TerminateThreads();
		void SyncInput();
		void SyncOutput();

	}; // End class Framework

} // End namespace Dataflow

#endif // FRAMEWORK_HPP
