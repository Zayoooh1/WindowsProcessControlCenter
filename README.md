# Windows Process Control Center (WPCC)

**A modern, lightweight Windows desktop shell for advanced process control and workflow optimization.**

## Vision: What is it & Why was it created?

Windows Process Control Center was built to give users granular, persistent control over their system resources without the bloat of traditional task managers. It is designed from the ground up to be incredibly fast, intuitively safe, and visually modern. Whether you are trying to squeeze every last frame out of a demanding game or ensuring your rendering software has the resources it needs, WPCC provides the control you need.

## Target Audience: Who is it for?

- **Gamers** wanting to eliminate stutters by forcing High CPU priority on heavy simulation or competitive games.
- **Motion designers and video editors** needing to strictly enforce discrete GPU (dGPU) usage for demanding rendering software.
- **Power users** who are obsessed with system optimization and background resource management.

## Core Features

- **Auto-Apply Engine**: Set your preferences once and let WPCC handle the rest. Our persistent profiles work automatically in the background to ensure your applications always run with your desired settings.
- **Modern WebView2 Interface**: A sleek, responsive, and resource-efficient user interface built on modern web technologies.
- **Per-App GPU Preferences**: Explicitly define which graphics processor (integrated or discrete) each application should use.
- **Built-in Safety Model**: Enjoy peace of mind knowing that WPCC's safety model protects critical Windows processes from being accidentally modified or terminated.

## Getting Started

Installation is simple and straightforward. No compilation or technical setup is required!

1. Navigate to the Releases page of this repository.
2. Download your preferred version:
   - **Standard Installer (.exe)**: For a typical installation experience.
   - **Portable Version (.zip)**: For running WPCC without installation.
3. Run the application and start optimizing!

## Known Limitations

- **GPU Preferences**: Applying new GPU preferences to an application may require that application to be restarted before the changes take effect.
- **Realtime Priority**: Setting an application to "Realtime" priority requires explicit confirmation, as doing so can pose risks to overall system stability and responsiveness.
