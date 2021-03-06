# FlowCL

For the forfilment of masters Computational Science of University van Amsterdam 2013
the author created a High-level OpenCL Framework using the dataflow model for ease of
application development, and introducing a novel API to create and prototype high 
performance applications using the dataflow model.

This framework aims to create an easy prototyping solution to OpenCL, automatically
handling ALL devices of MULTIPLE platforms automatically by abstracting devices.
By declaring Operations and their data dependencies as you would a dataflow graph,
the devices can be used in a directed acyclic graph, exploiting data and task
parallelism automatically.
Memory dependency and transfers are handled by best practices and memory concistency
is enforced depending on the dependencies of the graph.
This creates a novel way of programming complex algorithms using the dataflow concept
and easy prototyping.

What does FlowCL API look like?

    #include "FlowCL.hpp"
    
    void Main()
    {
		using namespace FlowCL;
			
		Context fcl;
		// Initialise and compile kernel for all available devices		
		fcl.CompileSource( "kernel void WorkId( global float* data )\
						    {\
								data[get_global_id(0)] = get_global_id(0);\
							}" );
		//std::cout << fcl.GetDebugInfo();  // Print general OpenCL platform & device info
		
		size_t big_mem = 1<<27;                     // 128mb
		size_t big_size = big_mem / sizeof(float);  // Number of floats in big_mem
		
		Memory mem_io = fcl.CreateMemory( big_mem );  // Create memory of size big_mem bytes
		float* data_out = (float*)mem_io.GetData();   // Get raw data pointer
		
		// Create operation that will run on the GPU device if availalbe
		Operation fo_one = fcl.CreateOperation( fcl.GetGPUDevice(), "WorkId" );
		fo_one.SetArgInput( 0, mem_io );  // Memory to be copied to the GPU
		fo_one.SetArgOutput( 0, mem_io ); // Memory to be copied back
		fo_one.SetWorkSize( big_size );   // Granularity of work items

		//std::cout << fcl.GetDebugInfo();
		
		fcl.Run(); // Run the created graph
		// data_out up to date, inspect contents
    }

This code snippet is simply greates a graph of one operation that will execute
the kernel function "WorkId" on the GPU while automatically handling the data according to the
dependencies.

The programmer could simply change fcl.GetGPUDevice() to fcl.GetCPUDevice()
if they want to use the CPU to run the operation instead.

Inter operation dependencies are simply declared by setting the argument
fo_one.SetDependency(parent_operation, index, memory), while the framework
handles the data transfer automatically.

Have a look at the [FlowCL application developers documentation](fcldocu.pdf)
for a quick overview of the features.

Tested and works on:

Windows 7, 8 with VS2012, Linux with GCC 4.7+, OpenCL v1.2

Compiler should have C++11 support.

This is a PRE-ALPHA project, and has not been extensively tested, only for proof of
conept.
