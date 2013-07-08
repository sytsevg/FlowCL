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

#ifndef DEVICE_HPP
#define DEVICE_HPP

namespace Dataflow
{
	
	class Device
	{

	public:

		Device( const cl::Device&  cl_device,
			    const cl::Context& cl_context, 
			    const cl::Program& cl_program );
		~Device();

		void AddMemoryObject( const cl::Buffer& cl_buf );

		const cl::Device&  GetCLDevice() const;
		const cl::Context& GetCLContext() const;
		const cl::Buffer&  GetCLMemory( const Memory* dev_mem ) const;
		const cl::Program& GetCLProgram() const;

		cl_device_id       GetCLDeviceID() const;
		cl_device_type     GetCLDeviceType() const;
		const std::string& GetTypeStr() const;
		const std::string& GetVendorStr() const;
		const std::string& GetNameStr() const;

		size_t             GetNumComputeUnits() const;
		size_t             GetMaxWorkGroupSize() const;
		size_t             GetMaxMemSize() const;

		bool               IsCPU() const;
		bool               IsGPU() const;
		bool               IsDedicatedDevice() const;
		bool               CanExecuteNativeKernel() const;

	private:

		// Device computation properties
		const std::string m_str_vendor;
		const std::string m_str_name;
		const std::string m_str_type;
		const size_t      m_num_compute_units;
		const size_t      m_max_work_group_size;
		const size_t      m_max_mem_size;
		const bool        m_exec_native_kernel;

		// OpenCL properties associated with device
		const cl::Context m_cl_context;
		const cl::Device  m_cl_device;
		const cl::Program m_cl_program;

   		std::string DeviceTypeToString( const cl::Device& cl_device ) const;

		// Allocated memory objects on this device
		std::vector<cl::Buffer> m_cl_mems;

	}; // End class Device

} // End namespace Dataflow

#endif //DEVICE_HPP