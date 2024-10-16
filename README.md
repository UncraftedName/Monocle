# PortalAngleGlitchTesting

A project for reproducing the Vertical Angle Glitch (VAG) bug in Portal 1.

## Background

VAG is a combination of several bugs that happens for some combinations of
portals when an entity gets very close to the portal boundary. One of the
causes of the bug is floating point rounding errors that happen when a portal
teleports an entity. Reproducing VAGs and predicting when they will happen
is a very frustrating part of speedrun routing.

The best tool we previously had for reproducing VAGs a an IPC script originally
written in [python](https://github.com/UncraftedName/portal1-python-scripts/blob/master/ipc_stuff/vag_searcher.py)
that was then ported to [SPT](https://github.com/YaLTeR/SourcePauseTool/blob/master/spt/features/vag_searcher.cpp)
which would try to teleport the player close to a portal to see if a VAG was
possibly for a specific portal combination. This approach was slow and often
reports false results.

## Goal

The goal of this project is to provide an environment outside of the game that
can reproduce the conditions needed to check if a VAG is possible on a specific
set of portals. To do this, some math functions had to be replicated in assembly.

It also had the goal of reproducing more complex chains of teleports, but that
is still very much WIP.

## Building

I'm using CMake with MSVC.
