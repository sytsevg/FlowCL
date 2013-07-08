# FlowCL

For the forfilment of masters Computational Science of University van Amsterdam
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

eg:

    #include "FlowCL.hpp"
    
    void Example()
    {
      using namespace FlowCL;
    
    	Context fcl;
      
      //std::cout << fcl.GetDebugInfo();  // Print general OpenCL platform & device info
    	//getchar();
    
    	fcl.CompileFile( file_path );
    	size_t big_mem = 1<<27;                       // 128mb
    	size_t big_size = big_mem / sizeof(float);
    
    	Memory mem_in = fcl.CreateMemory( big_mem );  // Create memory of size big_mem bytes
      Memory mem_out = fcl.CreateMemory( big_mem );
      float* data_out = mem_out.GetData();          // Get data pointer
    
      // Create operation that will run on the GPU device if availalbe
    	Operation fo_one = fcl.CreateOperation( fcl.GetGPUDevice(), "AddOne" );
      Operation fo_two = fcl.CreateOperation( fcl.GetCPUDebice(), "AddOne" );
      
      // Memory to be copied to the GPU
    	fo_one.SetArgInput( 0, mem_in );
      
      // Memory to be copied back
    	fo_one.SetArgOutput( 1, mem_out );
    
    
    	//Blocking run
    	fcl.Run();
    	
      // data_out up to date, inspect contents
    }



Note:

Compiler should have C++11 support.
This is a PRE-ALPHA project, and hasnt been extensively tested, only for proof of
conept.

Tested and works on Windows 7,8 with VS2012 and Linux with GCC 4.7+
