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
  Framework, higher level object oriented classes to create a dataflow OpenCL application

  Higher level layer for object oriented programming

  Author: S.D.M. van Geldermalsen
*/
#ifndef FLOWFRAMEWORK_HPP
#define FLOWFRAMEWORK_HPP

#if defined(_WIN32)
    #define CALLBACK __stdcall
#else
    #define CALLBACK
#endif

#include <typeinfo>
#include <vector>
#include <string>

namespace Dataflow
{
	// Class Forward Declarations
	class MemoryManager;
	class Device;
	class Operation;
	struct Memory;
}

namespace FlowCL
{
	// Class Forward Declarations
	class Context;
	class Device;
	class Memory;
	class Operation;

	// Definitions
	enum CopyInterval
	{
		CopyAlways = 0, CopyOnce, CopyNever,
	};
	
	class Context
	{
		public:

			Context();
			~Context();

			void CompileFile( const std::string& str_file_name );
			void CompileSource( const std::string& str_source );

			Memory CreateMemory( size_t size );
			Operation CreateOperation( const Device& dev, const std::string& kernel_name );
			Operation CreateOperation( const Device& dev, void (CALLBACK *user_func)(void*) );

			const Device& GetCPUDevice() const;
			const Device& GetGPUDevice() const;

			const std::string& GetDeviceInfo() const;
			const std::string& GetDebugInfo() const;

			const std::vector<Device>& Devices() const;

			void Run();

		private:

			std::vector<Device> m_devices;
	};

	class Device
	{
		friend class Context;
		friend class Operation;

		public:

			Device( const Dataflow::Device* dev );
			~Device();

			const std::string& GetName() const;
			const std::string& GetVendorName() const;
			bool IsCPUDevice() const;
			bool IsGPUDevice() const;
			bool IsDedicatedDevice() const;

        private:

			const Dataflow::Device* m_dev;
			const std::string& m_dev_name;
			const std::string& m_dev_vendor;
            bool m_iscpu;
            bool m_isgpu;
            bool m_isdedicated;
	};

	class Memory
	{
		friend class Operation;

		public:

			Memory( const Dataflow::Memory* dev_mem );
			~Memory();

			void* const GetData() const;
			size_t GetSize() const;

		private:

			void* const  m_data;
			const size_t m_size;

			const Dataflow::Memory* m_dev_mem;	
	};

	class Operation
	{
		public:

			Operation( Dataflow::Operation* op, const Device& dev );
			~Operation();

			void SetArg( size_t index, const Memory& mem );

			template <class T>
			void SetArgConstant( size_t index, T arg )
			{
				return SetArgConstant( index, sizeof(arg), (char*)typeid(arg).name(), (void*)&arg );
			}

			void SetArgInput( size_t index, const Memory& mem, CopyInterval copy_interval = CopyAlways );
			void SetArgOutput( size_t index, const Memory& mem, CopyInterval copy_interval = CopyAlways );
			void SetArgDependency( const Operation& dependency, size_t index, const Memory& mem, CopyInterval copy_interval = CopyAlways );

			void SetArgCopyInterval( size_t index, CopyInterval copy_interval );

			void SetWorkSize( size_t w );
			void SetWorkSize( size_t w, size_t h );
			void SetWorkSize( size_t w, size_t h, size_t d );

			const Device& GetDevice() const;

		private:

			const Device m_device;

			Dataflow::Operation* m_op;

			void SetArgConstant( size_t index, size_t size, const char* type, const void* arg );
	};

}

#endif // FLOWFRAMEWORK_HPP
