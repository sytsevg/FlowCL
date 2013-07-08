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

/* Test kernel for fun and no profit
   FML
*/
#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable

kernel void ComputeBound( global float* data, uint iter )    
{
	float frac;
    float sign = 1.0f;
    float odd  = 1.0f;
    float sum  = 0.0f;
    float res  = 0.0f;

	while( iter-- )
	{
		frac = sign / odd;
		sum += frac;
		res = 4.0f * sum;
		sign *= -1.0f;
		odd += 2.0f;
	}

	data[get_global_id(0)] = res;
}

kernel void AddNum( global float* src, global float* dst, float num )
{
	dst[get_global_id(0)] = src[get_global_id(0)] + num;
}

kernel void CopyOver( global float* src, global float* dst )
{
	dst[get_global_id(0)] = src[get_global_id(0)];
}

kernel void SetNum( global float4* set, float num )
{
	set[get_global_id(0)].xyzw = num;
}

kernel void Increment( global float4* inc, float num )
{
	inc[get_global_id(0)].xyzw += num;

}

inline uint GlobalIndex( uint x, uint y, uint z, uint w, uint h, uint d )
{
    return (z * (w *  h)) + (y * w) + x;
}

kernel void Sum( global int* matrix, volatile global uint* num_add_mean, volatile global uint* sum_add_mean, global float* region_mean  )
{
    // Global index
	uint g_x = get_global_id(0);
	uint g_y = get_global_id(1);
	uint g_z = get_global_id(2);
	uint g_i = GlobalIndex( g_x, g_y, g_z, get_global_size(0), get_global_size(1), get_global_size(2) );

    if( g_i == 0 )
	{
        *num_add_mean = 0;
        *sum_add_mean = 0;
	}

	barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

	(void)atomic_inc( num_add_mean );
    (void)atomic_add( sum_add_mean, matrix[g_i] );

	barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);

    if( g_i == (get_global_size(0) * get_global_size(1) * get_global_size(2)) - 1 )
	{
        *region_mean = (float)(*sum_add_mean) / (float)(*num_add_mean);
	}
}

// Using the max group size ^ 2, find lowest value 256 groups of <= 256 work items
kernel void MinGPU( global uint4* src,
				    global uint* gmin,
					uint nitems )
{
	uint count = ( nitems / 4 ) / get_global_size(0);
	uint idx = get_global_id(0);
	uint stride = get_global_size(0);
	uint pmin = (uint) -1;
	local uint lmin;

	for( uint n = 0; n < count; n++, idx += stride )
	{
		pmin = min( pmin, src[idx].x );
		pmin = min( pmin, src[idx].y );
		pmin = min( pmin, src[idx].z );
		pmin = min( pmin, src[idx].w );
	}

	if( get_local_id(0) == 0 )
	{
		lmin = (uint) -1;
	}
	
	barrier( CLK_LOCAL_MEM_FENCE );
	
	(void) atomic_min( &lmin, pmin );
	
	barrier( CLK_LOCAL_MEM_FENCE );

	if( get_local_id(0) == 0 )
	{
		gmin[ get_group_id(0) ] = lmin;
	}
}

kernel void ReduceMinGPU( global uint* gmin,
						  global uint* min )
{
	(void) atomic_min( min, gmin[get_global_id(0)] );
}

kernel void MaxGPU( global uint4* src,
				    global uint* gmax,
					uint nitems )
{
	uint count = ( nitems / 4 ) / get_global_size(0);
	uint idx = get_global_id(0);
	uint stride = get_global_size(0);
	uint pmax = 0;
	local uint lmax;

	for( uint n = 0; n < count; n++, idx += stride )
	{
		pmax = max( pmax, src[idx].x );
		pmax = max( pmax, src[idx].y );
		pmax = max( pmax, src[idx].z );
		pmax = max( pmax, src[idx].w );
	}

	if( get_local_id(0) == 0 )
	{
		lmax = 0;
	}
	
	barrier( CLK_LOCAL_MEM_FENCE );
	
	(void) atomic_max( &lmax, pmax );
	
	barrier( CLK_LOCAL_MEM_FENCE );

	if( get_local_id(0) == 0 )
	{
		gmax[ get_group_id(0) ] = lmax;
	}
}

kernel void ReducemaxGPU( global uint* gmax,
						  global uint* max )
{
	(void) atomic_max( max, gmax[get_global_id(0)] );
}

kernel void Nothing( global int* non )
{

}

kernel void NothingTwo( global float* one, global float* two )
{

}