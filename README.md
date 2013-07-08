FlowCL
======

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
