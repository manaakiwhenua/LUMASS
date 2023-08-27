---
title: "Aggregate Components"
permalink: "/docs/mod_aggregate_components"
--- 

## NMSequentialIterComponent

## NMParallelIterComponent

NMParallelIterComponent is an aggregate(-only) component that parallelizes a sequence of independent computations of a given model component over a given set of data and / or parameters (SPMD). Similar to NMSequentialIterComponent,
it iterates over a set of data and/or parameters and executes the given model with the defined data and/or parameters. However, in contrast to its sequential sibling, NMParallelIterComponent doesn't wait for the individual
computations to complete before launching the next iteration step (i.e. computation), hence executing all computations in parallel. Thus, all computations must be independent of each other and its produced data. 