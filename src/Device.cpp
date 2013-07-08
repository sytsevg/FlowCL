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
#include "Device.hpp"

namespace Dataflow
{

	Device::Device( const cl::Device& cl_device,
				    const cl::Context& cl_context, 
				    const cl::Program& cl_program )
					: m_str_vendor(cl_device.getInfo<CL_DEVICE_VENDOR>()),
				      m_str_name(cl_device.getInfo<CL_DEVICE_NAME>()),
					  m_str_type(DeviceTypeToString(cl_device)),
				      m_num_compute_units(cl_device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>()),
				      m_max_work_group_size(cl_device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>()),
				      m_max_mem_size((size_t)(cl_device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>())),
				      m_exec_native_kernel((cl_device.getInfo<CL_DEVICE_EXECUTION_CAPABILITIES>() & CL_EXEC_NATIVE_KERNEL) >= 1),
					  m_cl_context(cl_context),
                      m_cl_device(cl_device),
					  m_cl_program(cl_program)
	{

	}

	Device::~Device() { }

	void Device::AddMemoryObject( const cl::Buffer& cl_mem )
	{
		m_cl_mems.push_back( cl_mem );
	}

	cl_device_type Device::GetCLDeviceType() const
	{
		return m_cl_device.getInfo<CL_DEVICE_TYPE>();
	}

	const cl::Buffer& Device::GetCLMemory( const Memory* dev_mem ) const
	{
		return m_cl_mems[dev_mem->id];
	}

	const cl::Device& Device::GetCLDevice() const
	{
		return m_cl_device;
	}

	const cl::Context& Device::GetCLContext() const
	{
		return m_cl_context;
	}

	const cl::Program& Device::GetCLProgram() const
	{
		return m_cl_program;
	}

	cl_device_id Device::GetCLDeviceID() const
	{
		return m_cl_device();
	}

	bool Device::IsCPU() const
	{
		return GetCLDeviceType() == CL_DEVICE_TYPE_CPU;
	}

	bool Device::IsDedicatedDevice() const
	{
		return GetCLDeviceType() != CL_DEVICE_TYPE_CPU;
	}

	bool Device::IsGPU() const
	{
		return GetCLDeviceType() == CL_DEVICE_TYPE_GPU;
	}

	bool Device::CanExecuteNativeKernel() const
	{
		return m_exec_native_kernel;
	}

	std::string Device::DeviceTypeToString( const cl::Device& cl_device ) const
	{
		switch( cl_device.getInfo<CL_DEVICE_TYPE>() ) 
		{
			case CL_DEVICE_TYPE_CPU:
				return "CPU";
			case CL_DEVICE_TYPE_GPU:
				return "GPU";
			case CL_DEVICE_TYPE_ACCELERATOR:
				return "ACCELERATOR";
			case CL_DEVICE_TYPE_CUSTOM:
				return "CUSTOM";
			default:
				return "UNKNOWN";
		}
	}

	const std::string& Device::GetVendorStr() const
	{
		return m_str_vendor;
	}

	const std::string& Device::GetNameStr() const
	{
		return m_str_name;
	}

	const std::string& Device::GetTypeStr() const
	{
		return m_str_type;
	}

	size_t Device::GetNumComputeUnits() const
	{
		return m_num_compute_units;
	}

	size_t Device::GetMaxWorkGroupSize() const
	{
		return m_max_work_group_size;
	}

	size_t Device::GetMaxMemSize() const
	{
		return m_max_mem_size;
	}

} // End namespace Dataflow
