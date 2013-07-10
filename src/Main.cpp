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

#include <iostream>
#include <stdlib.h>
#include <sstream>

// Memory leak detection includes for visual studio
//#include "vld/vld.h"

// Project Includes
#include "FlowCL.hpp"


#define TEST_SIZE 50000000
#define TEST_SIZE_MEM TEST_SIZE * sizeof( float )

#ifdef _WIN32
const char* file_path = "src/kernel.cl";
#else
//sigh linux relative path woes
#endif

void FloatSet( float* data, float val, size_t len )
{
	for( size_t i = 0; i < len; i++ )
	{
		data[i] = val;
	}
}

void FloatSetInc( float* data, size_t len )
{
	for( size_t i = 0; i < len; i++ )
	{
		data[i] = (float)i;
	}
}

static void CALLBACK NativeKernelFunction( void* arg_ptr )
{
	void** args = (void**)arg_ptr;

	float* src  = (float*)args[0];
	float* dst  = (float*)args[1];
	float& add  = *(float*)args[2];
	
	for( size_t i = 0; i < TEST_SIZE; i++ )
	{
		dst[i] = src[i] + add;	
	}
}

static void CALLBACK NativeComputeBound( void* arg_ptr )
{
	void** args = (void**)arg_ptr;

	//#pragma omp parallel for
	for( int i = 0; i < TEST_SIZE; i++ )
	{
		int iter = 1000;
		float frac;
		float sign = 1.0f;
		float odd  = 1.0f;
		float sum  = 0.0f;
		float res  = 0.0f;
		float* data = (float*)args[0];

		while( iter-- )
		{
			frac = sign / odd;
			sum += frac;
			res = 4.0f * sum;
			sign *= -1.0f;
			odd += 2.0f;
		}

		data[i] = res;
	}
}

void AddNumTestCPUGPU()
{
	using namespace FlowCL;
	size_t test_size = 1000000;
    size_t test_size_mem = test_size * sizeof( float );
	float add_num_val = 1.0123f;

	Context fcl;
		
	fcl.CompileFile( file_path );

	Memory fm_in      = fcl.CreateMemory( test_size_mem );
	Memory fm_out_cpu = fcl.CreateMemory( test_size_mem );
	Memory fm_out_gpu = fcl.CreateMemory( test_size_mem );

	Operation fo_one = fcl.CreateOperation( fcl.GetGPUDevice(), "AddNum" );
	fo_one.SetArgInput( 0, fm_in, CopyOnce );
	fo_one.SetArgOutput( 1, fm_out_gpu, CopyNever );
	fo_one.SetArgConstant( 2, add_num_val);
	fo_one.SetWorkSize( test_size );

	Operation fo_two = fcl.CreateOperation( fcl.GetCPUDevice(), "AddNum" );
	fo_two.SetArgInput( 0, fm_in, CopyOnce );
	fo_two.SetArgOutput( 1, fm_out_cpu, CopyNever );
	fo_two.SetArgConstant( 2, add_num_val );
	fo_two.SetWorkSize( test_size );

	std::cout << fcl.GetDebugInfo();
	getchar();

	// Set some value to in
	FloatSetInc( (float*)fm_in.GetData(), test_size );
	// Clear output value
	FloatSet( (float*)fm_out_cpu.GetData(), 0, test_size );
	///Clear output value
	FloatSet( (float*)fm_out_gpu.GetData(), 0, test_size );

	//Blocking run
	for( size_t i = 0; i < 100; i++ )
	{
		fcl.Run();
	}

	fo_one.SetArgCopyInterval( 1, CopyOnce );
	fo_two.SetArgCopyInterval( 1, CopyOnce );

	fcl.Run();

	for( size_t i = 0; i < 20; i++ )
	{
		std::cout << "fm_out_cpu[" << i << "] = " << ((float*)fm_out_cpu.GetData())[i] << std::endl;
	}

	for( size_t i = 0; i < 20; i++ )
	{
		std::cout << "fm_out_gpu[" << i << "] = " << ((float*)fm_out_gpu.GetData())[i] << std::endl;
	}
}

void IncrTestCPUGPU()
{
	using namespace FlowCL;
	size_t test_size = 1000000;
    size_t test_size_mem = test_size * sizeof( float );
	float add_num_val = 1.0123f;

	Context fcl;
		
	fcl.CompileFile( file_path );

	Memory fm_out_cpu = fcl.CreateMemory( test_size_mem );
	Memory fm_out_gpu = fcl.CreateMemory( test_size_mem );

	Operation fo_one = fcl.CreateOperation( fcl.GetGPUDevice(), "Increment" );
	fo_one.SetArgOutput( 0, fm_out_gpu, CopyNever );
	fo_one.SetArgConstant( 1, add_num_val );
	fo_one.SetWorkSize( test_size );

	Operation fo_two = fcl.CreateOperation( fcl.GetCPUDevice(), "Increment" );
	fo_two.SetArgOutput( 0, fm_out_cpu, CopyNever );
	fo_two.SetArgConstant( 1, add_num_val );
	fo_two.SetWorkSize( test_size );

	//std::cout << fcl.GetDebugInfo();

	// Clear output value
	FloatSet( (float*)fm_out_cpu.GetData(), 0, test_size );
	///Clear output value
	FloatSet( (float*)fm_out_gpu.GetData(), 0, test_size );

	//Blocking run
	for( size_t i = 0; i < 99; i++ )
	{
		fcl.Run();
	}

	fo_one.SetArgCopyInterval( 0, CopyOnce );
	fo_two.SetArgCopyInterval( 0, CopyOnce );

	fcl.Run();

	for( size_t i = 0; i < 20; i++ )
	{
		std::cout << "fm_out_cpu[" << i << "] = " << ((float*)fm_out_cpu.GetData())[i] << std::endl;
	}

	for( size_t i = 0; i < 20; i++ )
	{
		std::cout << "fm_out_gpu[" << i << "] = " << ((float*)fm_out_gpu.GetData())[i] << std::endl;
	}
}

void OverlapMemoryTransferGPU()
{
	using namespace FlowCL;
	Context fcl;
		
	fcl.CompileFile( file_path );

	size_t big_size = 50000000;
	size_t big_mem = big_size * sizeof(int);

	Memory fm_in_gpu = fcl.CreateMemory( big_mem );
	Memory fm_out_gpu = fcl.CreateMemory( big_mem );

	Operation fo_one = fcl.CreateOperation( fcl.GetGPUDevice(), "Nothing" );
	fo_one.SetArgInput( 0, fm_in_gpu );
	fo_one.SetWorkSize( 0 );

	Operation fo_two = fcl.CreateOperation( fcl.GetGPUDevice(), "Nothing" );
	fo_two.SetArgOutput( 0, fm_out_gpu );
	fo_two.SetWorkSize( 0 );
	
	Operation fo_three = fcl.CreateOperation( fcl.GetGPUDevice(), "Nothing" );
	fo_three.SetArgInput( 0, fm_in_gpu, CopyNever );
	fo_three.SetWorkSize( 0 );
	
	Operation fo_four = fcl.CreateOperation( fcl.GetCPUDevice(), "Nothing" );
	fo_four.SetArgDependency( fo_three, 0, fm_in_gpu, CopyNever );
	fo_four.SetWorkSize( 0 );

	fcl.Run();
	fcl.Run();
	fcl.Run();
	fcl.Run();
	fcl.Run();
	fcl.Run();
	fcl.Run();
	fcl.Run();

	//Blocking run
	for( size_t i = 0; i < 20; i++ )
	{
		fcl.Run();
	}

	fo_one.SetArgCopyInterval( 0, CopyNever );
	fo_two.SetArgCopyInterval( 0, CopyNever );

	fo_three.SetArgCopyInterval( 0, CopyAlways );
	fo_four.SetArgCopyInterval( 0, CopyAlways );

	//Blocking run
	for( size_t i = 0; i < 20; i++ )
	{
		fcl.Run();
	}
}

void ComputeBoundTestCPUGPU()
{
	using namespace FlowCL;
	Context fcl;
		
	fcl.CompileFile( file_path );
	
	size_t big_size = 26214400;
	size_t big_mem = big_size * sizeof(float);

	Memory fm_out_cpu = fcl.CreateMemory( big_mem );
	Memory fm_out_gpu = fcl.CreateMemory( big_mem );
	
	Operation fo_one = fcl.CreateOperation( fcl.GetGPUDevice(), "ComputeBound" );
	fo_one.SetArgOutput( 0, fm_out_gpu, CopyNever );
	fo_one.SetArgConstant( 1, (int)10 );
	fo_one.SetWorkSize( big_size );

	Operation fo_two = fcl.CreateOperation( fcl.GetCPUDevice(), "ComputeBound" );
	fo_two.SetArgOutput( 0, fm_out_cpu, CopyNever );
	fo_two.SetArgConstant( 1, (int)10 );
	fo_two.SetWorkSize( big_size );

	// Clear output value
	FloatSet( (float*)fm_out_cpu.GetData(), 0, big_size );
	// Clear output value
	FloatSet( (float*)fm_out_gpu.GetData(), 0, big_size );

	//std::cout << fcl.GetDebugInfo();
	//getchar();

	//Blocking run
	for( size_t i = 0; i < 9; i++ )
	{
		fcl.Run();
	}

	fo_one.SetArgCopyInterval( 0, CopyOnce );
	fo_two.SetArgCopyInterval( 0, CopyOnce );

	fcl.Run();

	for( size_t i = 0; i < 20; i++ )
	{
		std::cout << "fm_out_cpu[" << i << "] = " << ((float*)fm_out_cpu.GetData())[i] << std::endl;
	}

	for( size_t i = 0; i < 20; i++ )
	{
		std::cout << "fm_out_gpu[" << i << "] = " << ((float*)fm_out_gpu.GetData())[i] << std::endl;
	}
}

void LargeDataTransfer()
{
	using namespace FlowCL;
	Context fcl;

	fcl.CompileFile( file_path );
	size_t big_size = 26214400;
	//size_t big_size = 512*512*30*2;
	size_t big_mem = big_size * sizeof(float);

	Memory fm_in_gpu = fcl.CreateMemory( big_mem );
	Memory fm_out_gpu = fcl.CreateMemory( big_mem );

	Operation fo_one = fcl.CreateOperation( fcl.GetGPUDevice(), "CopyOver" );
	fo_one.SetArgInput( 0, fm_in_gpu );
	fo_one.SetArgOutput( 1, fm_out_gpu );
	fo_one.SetWorkSize( big_size );

	//std::cout << fcl.GetDebugInfo();
	//getchar();

	//Blocking run
	for( size_t i = 0; i < 20; i++ )
	{
        //std::cout << "Iteration " << i << std::endl;
		fcl.Run();
	}
}

void DoubleLargeDataTransfer()
{
	using namespace FlowCL;

	Context fcl;

	fcl.CompileFile( file_path );
	size_t big_mem = 1<<27;             // 128mb
	size_t big_size = big_mem / sizeof(float);

	Memory fm_in_gpu_one = fcl.CreateMemory( big_mem );
	Memory fm_in_gpu_two = fcl.CreateMemory( big_mem );

	Memory fm_out_gpu_one = fcl.CreateMemory( big_mem );
	Memory fm_out_gpu_two = fcl.CreateMemory( big_mem );

	Operation fo_one = fcl.CreateOperation( fcl.GetGPUDevice(), "NothingTwo" );
	fo_one.SetArgInput( 0, fm_in_gpu_one );
	fo_one.SetArgInput( 1, fm_in_gpu_two );
	fo_one.SetArgOutput( 0, fm_out_gpu_one );
	fo_one.SetArgOutput( 1, fm_out_gpu_one );

	//std::cout << fcl.GetDebugInfo();
	//getchar();

	//Blocking run
	for( size_t i = 0; i < 20; i++ )
	{
        //std::cout << "Iteration " << i << std::endl;
		fcl.Run();
	}

}

void SetOneAddGPUAddCPU()
{
	using namespace FlowCL;
	Context fcl;

	fcl.CompileFile( file_path );

	Memory fm_out = fcl.CreateMemory( TEST_SIZE_MEM );

	Operation fo_one = fcl.CreateOperation( fcl.GetGPUDevice(), "SetNum" );
	fo_one.SetArg( 0, fm_out );
	fo_one.SetArgConstant( 1, (float)1 );
	fo_one.SetWorkSize( TEST_SIZE / 4 );

	Operation fo_two = fcl.CreateOperation( fcl.GetGPUDevice(), "Increment" );
	fo_two.SetArgDependency( fo_one, 0, fm_out );
	fo_two.SetArgConstant( 1, (float)2 );
	fo_two.SetWorkSize( TEST_SIZE / 4 );

	Operation fo_three = fcl.CreateOperation( fcl.Devices()[1], "Increment" );
	fo_three.SetArgDependency( fo_two, 0, fm_out );
	fo_three.SetArgConstant( 1, (float)3 );
	fo_three.SetWorkSize( TEST_SIZE / 4 );

	//Blocking run
	for( size_t i = 0; i < 10; i++ )
	{
		fcl.Run();
	}

	//for( size_t i = 0; i < 20; i++ )
	//{
	//	std::cout << "fm_out[" << i << "] = " << ((float*)fm_out.GetData())[i] << std::endl;
	//}

	//getchar();
}

void MeanTest()
{
	using namespace FlowCL;
	Context fcl;
		
	fcl.CompileFile( file_path );

	const size_t w = 100;
	const size_t h = 100;

	size_t memsize = w * h * sizeof(int);

	Memory fm_matrix = fcl.CreateMemory( memsize );
	Memory fm_num_add_mean = fcl.CreateMemory( sizeof(unsigned int) );
	Memory fm_sum_add_mean = fcl.CreateMemory( sizeof(unsigned int) );
	Memory fm_region_mean = fcl.CreateMemory( sizeof(float) );

	Operation fo_one = fcl.CreateOperation( fcl.GetGPUDevice(), "ValIndex" );
	fo_one.SetArgConstant( 0, fm_matrix );
	fo_one.SetWorkSize( w, h );

	Operation fo_two = fcl.CreateOperation( fcl.GetGPUDevice(), "Sum" );
	fo_two.SetArgDependency( fo_one, 0, fm_matrix );
	fo_two.SetArgConstant( 1, fm_num_add_mean );
	fo_two.SetArgConstant( 2, fm_sum_add_mean );
	fo_two.SetArgConstant( 3, fm_region_mean );
	fo_two.SetWorkSize( w, h );

	//Blocking run
	fcl.Run();

	int (*matrix)[h][w] = (int (*)[h][w])fm_matrix.GetData();

	size_t region_size = 0;
	size_t sum = 0;
	float mean = 0;

	for( size_t y = 0; y < w; y++ )
	{
		for( size_t x = 0; x < h; x++ )
		{
			(*matrix)[y][x] = y * w + x;
			sum += (*matrix)[y][x];
		}
	}

	for( size_t y = 0; y < w; y++ )
	{
		for( size_t x = 0; x < h; x++ )
		{
			mean = ((float)mean * (float)region_size + (float)(*matrix)[y][x]) / (float)(region_size+1);
			region_size++;
		}
	}

	//unsigned int* gpu_sum = (unsigned int*)fm_sum_add_mean.GetData();
	//unsigned int* gpu_num = (unsigned int*)fm_num_add_mean.GetData();
	//float* gpu_mean = (float*)fm_region_mean.GetData();
}

int main()
{  


	using namespace FlowCL;

	try
	{
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
		data_out[0] = 1;
		data_out[1] = 2;
		
		// Create operation that will run on the GPU device if availalbe
		Operation fo_one = fcl.CreateOperation( fcl.GetGPUDevice(), "WorkId" );
		fo_one.SetArgInput( 0, mem_io );  // Memory to be copied to the GPU
		fo_one.SetArgOutput( 0, mem_io ); // Memory to be copied back
		fo_one.SetWorkSize( big_size );   // Granularity of work items

		//std::cout << fcl.GetDebugInfo();
		
		fcl.Run(); // Run the created graph
		// data_out up to date, inspect contents

		//Various tests

		//AddNumTestCPUGPU();
		//IncrTestCPUGPU();
		//ComputeBoundTestCPUGPU();
		//LargeDataTransfer();
		//DoubleLargeDataTransfer();
		//OverlapMemoryTransferGPU();
		//SetOneAddGPUAddCPU();
		//MeanTest();
		//OverheadTest();
		//EditingVolumeStuff();
	}
	catch( std::exception& e )
	{
		std::cout << e.what();
	}

	//getchar();

	//std::cout << "Done." << std::endl;

    return 0;
}
