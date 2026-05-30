# Windows Process Control Center (WPCC)

**A modern, lightweight Windows optimization program and desktop shell for advanced process control and performance tuning.**

<img src="screenshots/showcase.gif" width="100%" alt="WPCC Showcase">

## Vision: What is it & Why was it created?

Windows Process Control Center (WPCC) was built to give users granular, persistent control over their system resources without the bloat of traditional utilities. As a dedicated **Windows optimization program**, it is designed from the ground up to be incredibly fast, intuitively safe, and visually modern. Whether you are trying to squeeze every last frame out of a demanding game or ensuring your rendering software has the resources it needs, WPCC provides the system optimization you need to maximize PC performance.

## Target Audience: Who is it for?

Whether you are looking for a reliable **PC performance optimizer** to eliminate background lag or a specialized tool for resource allocation, WPCC is tailored for:

- **Gamers** wanting to eliminate stutters and maximize gaming performance by forcing High CPU priority on heavy simulation or competitive games.
- **Motion designers and video editors** needing to strictly enforce discrete GPU (dGPU) usage for demanding rendering software.
- **Power users** who are obsessed with Windows system optimization, debloating, and advanced background resource management.

## Core Features

- **Auto-Apply Engine**: Set your performance preferences once and let WPCC handle the rest. Our persistent profiles work automatically in the background to ensure your applications always run with your desired settings.
- **Modern WebView2 Interface**: A sleek, responsive, and resource-efficient user interface built on modern web technologies.
- **Per-App GPU Preferences**: Explicitly define which graphics processor (integrated or discrete) each application should use to optimize power and speed.
- **Built-in Safety Model**: Enjoy peace of mind knowing that WPCC's safety model protects critical Windows system processes from being accidentally modified or terminated.

---

## Getting Started

Installation is simple and straightforward. No compilation or complex technical setup is required!

1. Navigate to the [Releases](../../releases) page of this repository.
2. Download your preferred version:
   - **Standard Installer (.exe)**: For a typical installation experience.
   - **Portable Version (.zip)**: For running WPCC without installation.
3. Run the application and start optimizing your Windows performance!

## Known Limitations

- **GPU Preferences**: Applying new GPU preferences to an application may require that application to be restarted before the changes take effect.
- **Realtime Priority**: Setting an application to "Realtime" priority requires explicit confirmation, as doing so can pose risks to overall system stability and responsiveness.
