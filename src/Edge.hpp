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
 Edge

 Represents an edge of a data flow memory token. Memory will "flow" through this
 edge to another "operation" I.E. an Operation.

 Author: S.D.M. van Geldermalsen

 Date: 13-09-2012
 */

#ifndef EDGE_HPP
#define EDGE_HPP

namespace Dataflow
{

	class Edge
	{

	public:

		Edge( const Operation* op_input,
			  const Operation* op_output,
			  const Memory* dev_mem,
			  const EdgeType type,
			  CopyInterval copy_interval );
		~Edge();

		void SetInputSync( SyncBarrier* in );
		void SetOutputSync( SyncBarrier* out );

		const Memory* GetMemory();
		void SetCopyInterval( CopyInterval copy_interval );
		CopyInterval GetCopyInterval();
	
		void ThreadStart( SyncStart* start, bool* execute );

		const std::string& GetDebugInfo();

	private:

		// The edge is connected between two pc's
		const Operation* m_op_input;    // may be NULL if Memory's type is InputMemory
		const Operation* m_op_output;   // may be NULL if Memory's type is OutputMemory

		// The memory that will be transferred 
		const Memory* m_dev_mem;

		// Compute Devices memory will be transfered from/to
		const Device* m_dev_input;
		const Device* m_dev_output;

		// Memory queues of devices
		cl::CommandQueue m_cl_queue_input;
		cl::CommandQueue m_cl_queue_output;

		// Event for profiling
		cl::Event m_event;

		// Type of edge
		const EdgeType m_type;
		CopyInterval   m_copy_interval;

		// Synchronization
		SyncBarrier* m_sync_input;
		SyncBarrier* m_sync_output;

		// Debug string data
		std::string m_string_debug;

		void SyncInput();
		void SyncOutput();

		void TransferMemory();
		void HandleHostToDeviceTransfer();
		void HandleDeviceToHostTransfer();
		void HandleDeviceToDeviceTransfer();

		void PrintProfile();
	
	}; // End class Edge

} // End namespace Processing Framework

#endif //EDGE_HPP
