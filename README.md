# FlowCL

For the forfilment of masters Computational Science of University van Amsterdam 2013
I created a High-level OpenCL Framework using the dataflow model for ease of
application development, and using the dataflow model of execution.

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
		//std::cout << fcl.GetDebugInfo();  // Print general OpenCL platform & device info
		
		fcl.CompileFile( file_path );       // Compile for all available devices
		
		size_t big_mem = 1<<27;                     // 128mb
		size_t big_size = big_mem / sizeof(float);  // Number of floats in big_mem
		
		Memory mem_in = fcl.CreateMemory( big_mem );  // Create memory of size big_mem bytes
		Memory mem_out = fcl.CreateMemory( big_mem );
		
		float* data_out = (float*)mem_out.GetData();  // Get raw data pointer
		
		// Create operation that will run on the GPU device if availalbe
		Operation fo_one = fcl.CreateOperation( fcl.GetGPUDevice(), "AddOne" );
		fo_one.SetArgInput( 0, mem_in );   // Memory to be copied to the GPU
		fo_one.SetArgOutput( 1, mem_out ); // Memory to be copied back
		fo_one.SetWorkSize( big_size );    // Granularity of work items
		
		fcl.Run(); // Run the created graph
		// data_out up to date, inspect contents
    }

This code snippet is simply greates a graph of one operation that will execute
the kernel on the GPU while automatically handling the data according to the
dependencies.

The programmer could simply change fcl.GetGPUDevice() to fcl.GetCPUDevice()
if they want to use the CPU instead.

Inter operation dependencies are simply declared by setting the argument
fo_one.SetDependency(parent operation, index, memory), while the framework
handles the data transfer automatically.

Have a look at the [FlowCL application developers documentation](fcldocu.pdf)
for a quick overview of the features.

Tested and works on:

Windows 7, 8 with VS2012, Linux with GCC 4.7+, OpenCL v1.2

Compiler should have C++11 support.

This is a PRE-ALPHA project, and has not been extensively tested, only for proof of
conept.
