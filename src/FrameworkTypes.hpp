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
  Framework Types

  Defines commonly used types between classes

  Author: S.D.M. van Geldermalsen

  */
#ifndef FRAMEWORK_TYPES_HPP
#define FRAMEWORK_TYPES_HPP

// STD Includes widely used in project
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

//#define PROFILE_FRAMEWORK
//#define PROFILE_EDGE
//#define PROFILE_OPERATION

namespace Dataflow
{
	// Class Forward declarations
	class Framework;
	class MemoryManager;
	class Device;
	class Operation;
	class SyncBarrier;
	class SyncStart;
	class Edge;
	
	struct Memory
	{
		const size_t           size;
		void* const            ptr_mem;
		const size_t           id;

		Memory( size_t s, void* const p, size_t i )
					  : size(s), ptr_mem(p), id(i) { }
	};

	enum EdgeType
	{
		NoOp = 0, HostToDevice, DeviceToHost, DeviceToDevice,
	};

	const char* const EdgeTypeString[] = { "NoOp", "HostToDevice", "DeviceToHost", "DeviceToDevice" };

	enum CopyInterval
	{
		Always = 0, Once, Never,
	};

	const char* const CopyIntervalString[] = { "Always", "Once", "Never" };
	
} // End namespace Dataflow

#endif // FRAMEWORK_TYPES_HPP
