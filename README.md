# Replicated Sublevel Instances

Dynamically instantiate sublevels in multiplayer games. Great for procedurally generated worlds!

## About

If you've got a procedurally generated multiplayer game, this is the plugin for you. It allows you to spawn sublevel instances at runtime and have them replicated across the network to all clients. By default Unreal Engine does not support this behaviour (dynamic sublevel instances are not replicated).

This works by introducing a new component you can use in actors: the **Sublevel** component. Attach this component to a replicated actor, set the level path and enable Level Active, and it will handle replicating a dynamic level instance to all clients. You can use as many sublevel components as you like, which makes this a great plugin if your game implements procedural generation by stitching sublevels together.

## No support

This plugin has now been retired, and will no longer receive updates or support. Please note that attribution is still required; if you use this plugin in your game, you must include a [copy of the MIT license with the copyright header](LICENSE) in your distribution.