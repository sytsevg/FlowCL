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
  Processing Framework client include file

  Higher level layer for object oriented programming

  Author: S.D.M. van Geldermalsen
*/

// Standard includes
#include <stdexcept>
#include <thread>

// OpenCL includes
#define __CL_ENABLE_EXCEPTIONS
#include "CL/cl.hpp"

// Project includes
#include "FrameworkTypes.hpp"
#include "Synchronization.hpp"
#include "MemoryManager.hpp"
#include "Device.hpp"
#include "Operation.hpp"
#include "Framework.hpp"
#include "FlowCL.hpp"

namespace Dataflow
{
	class Framework;
}

namespace FlowCL
{
	// Global pointer to Dataflow::ramework used by Flow::Framework and Flow::Operation
	static Dataflow::Framework* g_df = nullptr;

	// FRAMEWORK -------------------------------------------------------------
	Context::Context()
	{
		g_df = new Dataflow::Framework();
	}

	Context::~Context()
	{
		if( g_df )
		{
			delete g_df;
		}
	}

	void Context::CompileFile( const std::string& str_file_name )
	{
		g_df->CompileFile(str_file_name);
		
		std::vector<Dataflow::Device*> devs = g_df->GetDevices();

		for( Dataflow::Device* dev : devs )
		{
			m_devices.push_back( Device(dev) );
		}
	}

	void Context::CompileSource( const std::string& str_source )
	{
		g_df->CompileSource(str_source);
		
		std::vector<Dataflow::Device*> devs = g_df->GetDevices();

		for( Dataflow::Device* dev : devs )
		{
			m_devices.push_back( Device(dev) );
		}
	}

	const std::vector<Device>& Context::Devices() const
	{
		return m_devices;
	}

	Memory Context::CreateMemory( size_t size )
	{
		return Memory( g_df->CreateMemory(size) );
	}

	Operation Context::CreateOperation( const Device& dev, const std::string& kernel_name )
	{
		return Operation( g_df->CreateDeviceOperation(dev.m_dev, kernel_name), dev );
	}

	Operation Context::CreateOperation( const Device& dev, void (CALLBACK *user_func)(void*) )
	{
		return Operation( g_df->CreateNativeOperation(dev.m_dev, user_func), dev );
	}

	const Device& Context::GetCPUDevice() const
	{
		for( const Device& dev : m_devices )
		{
			if( dev.IsCPUDevice() )
			{
				return dev;
			}
		}

		throw std::runtime_error("No CPU device found");
	}

	const Device& Context::GetGPUDevice() const
	{
		for( const Device& dev : m_devices )
		{
			if( dev.IsGPUDevice() )
			{
				return dev;
			}
		}

		throw std::runtime_error("No GPU device found");
	}

	const std::string& Context::GetDeviceInfo() const
	{
		//std::vector<Dataflow::Device*> df_devs = g_df->GetDevices();
		//std::vector<Device> devs;

		//for( Dataflow::Device* dev : df_devs )
		//{
		//	devs.push_back( Device(dev) );
		//}

		static std::string result;
		std::stringstream ss;
		size_t i = 0;

		ss << "FlowCL usable devices\n\n";

		for( const Device& dev : m_devices )
		{
			ss << "\t" << i++;

			if( dev.IsCPUDevice() )
			{
				ss << " CPU, ";
			}
			else if( dev.IsGPUDevice() )
			{
				ss <<  " GPU, ";
			}
			else
			{
				ss << " Custom, ";
			}

			ss << "VENDOR: " << dev.GetVendorName() << " DEVICE: " << dev.GetName() << "\n";
		}
		
		return result = ss.str();

		//return g_df->GetDeviceInfo();
	}

	const std::string& Context::GetDebugInfo() const
	{
		return g_df->GetDebugInfo();
	}

	void Context::Run()
	{
		g_df->Run();
	}
	// FRAMEWORK -------------------------------------------------------------

	// DEVICE ----------------------------------------------------------------
	Device::Device( const Dataflow::Device* dev )
				   : m_dev(dev),
				     m_dev_name(dev->GetNameStr()),
					 m_dev_vendor(dev->GetVendorStr()),
				     m_iscpu(dev->IsCPU()),
					 m_isgpu(dev->IsGPU()),
					 m_isdedicated(dev->IsDedicatedDevice())
	{

	}

	Device::~Device() { }

	//Device& Device::operator= ( const Device& rhs )
	//{
	//	m_dev = rhs.m_dev;
	//	m_iscpu = &rhs.m_iscpu;
	//	m_isgpu = &rhs.m_isgpu;
	//	m_isdedicated = &rhs.m_isdedicated;

	//	return *this;
	//}

	bool Device::IsCPUDevice() const
	{
		return m_iscpu;
	}

	bool Device::IsGPUDevice() const
	{
		return m_isgpu;
	}

	bool Device::IsDedicatedDevice() const
	{
		return m_isdedicated;
	}

	const std::string& Device::GetVendorName() const
	{
		return m_dev_vendor;
	}

	const std::string& Device::GetName() const
	{
		return m_dev_name;
	}

	// DEVICE ----------------------------------------------------------------

	// MEMORY ----------------------------------------------------------------
	Memory::Memory( const Dataflow::Memory* dev_mem )
                    : m_data(dev_mem->ptr_mem),
                      m_size(dev_mem->size),
                      m_dev_mem(dev_mem)
	{

	}

	Memory::~Memory() { }

	void* const Memory::GetData() const
	{
		return m_data;
	}

	size_t Memory::GetSize() const
	{
		return m_size;
	}

	// MEMORY ----------------------------------------------------------------

	// OPERATION -------------------------------------------------------------
	Operation::Operation( Dataflow::Operation* op, const Device& dev )
		                  : m_device(dev),
                            m_op(op)
	{

	}

	Operation::~Operation() { }

	void Operation::SetArg( size_t index, const Memory& mem )
	{
		g_df->SetOperationArgMemory( m_op, index, mem.m_dev_mem );
	}

	void Operation::SetArgConstant( size_t index, size_t size, const char* type, const void* arg )
	{
		g_df->SetOperationArgConstant( m_op, index, size, type, arg );
	}

	void Operation::SetArgInput( size_t index, const Memory& mem, CopyInterval copy_interval )
	{
		g_df->SetOperationArgInput( m_op, index, mem.m_dev_mem, (Dataflow::CopyInterval)copy_interval );
	}

	void Operation::SetArgOutput( size_t index, const Memory& mem, CopyInterval copy_interval )
	{
		g_df->SetOperationArgOutput( m_op, index, mem.m_dev_mem, (Dataflow::CopyInterval)copy_interval );
	}

	void Operation::SetArgDependency( const Operation& dependency, size_t index, const Memory& mem, CopyInterval copy_interval )
	{
		g_df->SetOperationArgDependency( m_op, dependency.m_op, index, mem.m_dev_mem, (Dataflow::CopyInterval)copy_interval );
	}

	void Operation::SetArgCopyInterval( size_t index, CopyInterval copy_interval )
	{
		g_df->SetOperationArgMemoryCopyInterval( m_op, index, (Dataflow::CopyInterval)copy_interval );
	}

	void Operation::SetWorkSize( size_t w )
	{
		g_df->SetOperationWorkSize( m_op, cl::NDRange(w), cl::NullRange );
	}

	void Operation::SetWorkSize( size_t w, size_t h )
	{
		g_df->SetOperationWorkSize( m_op, cl::NDRange(w, h), cl::NullRange );
	}

	void Operation::SetWorkSize( size_t w, size_t h, size_t d )
	{
		g_df->SetOperationWorkSize( m_op, cl::NDRange(w, h, d), cl::NullRange );
	}
	// OPERATION -------------------------------------------------------------

} // End namespace Flow
