# Pacman GUI

<div align="center">
  <img src="icon.svg" alt="Pacman GUI Icon" width="128" height="128">
</div>

A modern, fast, and intuitive GTK4 graphical interface for pacman package manager with AUR support for Arch Linux.

## Features

### Package Management
- 🔍 **Search packages** in official repositories and AUR
- 📦 **Install/Remove packages** with real-time logs
- 🗂️ **Installed packages management** with fast async loading and tabbed interface
- 🔄 **System updates** with progress tracking

### Advanced Features
- 📊 **Package dependency visualization** with interactive graph viewer
- 🧹 **Package cache cleanup** - remove old packages or clear entire cache
- ⚡ **Async loading with spinners** - no UI freezing, smart lazy loading
- 📋 **Live operation logs** in separate window with timestamps

### User Experience
- 🏃 **Fast startup** - instant launch with background loading
- 🎨 **Modern tabbed interface** - separate search and installed package views
- 🌙 **Dark theme support** - follows system theme automatically
- 🛠️ **AUR support** via yay or paru detection
- 🎯 **Beginner-friendly** with progress indicators

## Installation

### From AUR
```bash
yay -S pacman-gui
# or
paru -S pacman-gui
```

### Build from source
```bash
# Dependencies
sudo pacman -S gtk4 glib2 cmake gcc pkgconf

# Clone and build
git clone https://github.com/Coneriys/pacman-gui.git
cd pacman-gui
mkdir build && cd build
cmake ..
make

# Install
sudo make install
```

## Dependencies

### Required
- `gtk4` - GUI toolkit
- `glib2` - GLib library
- `pacman` - Package manager
- `polkit` - Privilege escalation

### Optional
- `yay` - AUR helper (recommended)
- `paru` - Alternative AUR helper

## Usage

### Launch
```bash
pacman-gui
```
Or find "Pacman GUI" in your application menu.

### Basic Operations

#### Search & Install Packages
1. **Search packages**: Enter package name in search tab and click Search
2. **Choose source**: Select "Official Repos" or "AUR" from dropdown
3. **Install**: Select package from list and click Install
4. **View dependencies**: Select package and click "Dependencies" for interactive graph

#### Manage Installed Packages
1. **Browse installed**: Switch to "Installed Packages" tab (loads on first visit)
2. **Remove packages**: Select installed package and click Remove
3. **Refresh list**: Click "Refresh Installed Packages" to update

#### System Maintenance
1. **Update system**: Click "Update System" button for full system upgrade
2. **Clean cache**: Use "Clean Cache" to remove old packages or "Clean All Cache" for complete cleanup
3. **Monitor operations**: All operations show real-time logs in a separate window

### AUR Support

The application automatically detects installed AUR helpers (yay/paru). If none found, AUR search will be disabled.

## Architecture

```
src/
├── main.c              # Application entry point
├── pacman_wrapper.c    # Package manager backend
├── pacman_wrapper.h    # Backend interface
└── ui/
    ├── main_window.c       # Main GUI implementation
    ├── main_window.h       # GUI interface
    ├── dependency_viewer.c # Dependency visualization component
    └── dependency_viewer.h # Dependency viewer interface
```

### Key Components

- **pacman_wrapper**: Handles pacman/AUR operations, dependency parsing, and async package loading
- **main_window**: Modern tabbed GTK4 interface with async loading, spinners, and real-time logs
- **dependency_viewer**: Interactive dependency graph visualization with Cairo rendering
- **Async operations**: Non-blocking package operations with background threads and UI feedback
- **Smart loading**: Lazy loading of installed packages with smooth transitions and progress indicators

## Configuration

The application automatically:
- Detects system theme (light/dark)
- Finds available AUR helpers
- Uses `pkexec` for privilege escalation

## Development

### Building
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

### Contributing

1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open Pull Request

### Code Style
- C11 standard
- 4-space indentation
- GTK4 conventions
- Comment public functions

## Why This Project?

Arch Linux is powerful but can be intimidating for newcomers. This GUI provides:

- **Accessibility**: No need to memorize pacman commands
- **Safety**: Visual confirmation before operations
- **Learning**: See actual commands being executed
- **Convenience**: AUR integration without manual setup
- **Performance**: Fast, responsive experience even on older hardware

## Roadmap

- [x] Package dependency visualization
- [x] Installed packages management
- [x] Package cache cleanup
- [x] Async loading with preloaders
- [ ] Package categories/groups
- [ ] Configuration settings dialog
- [ ] Repository management
- [ ] Multiple language support
- [ ] Package information details view
- [ ] Update notifications

## License

GPL-3.0 License. See [LICENSE](LICENSE) for details.

## Acknowledgments

- Arch Linux community
- GTK development team
- AUR helper developers (yay, paru)

## Support

- **Issues**: [GitHub Issues](https://github.com/Coneriys/pacman-gui/issues)
- **Wiki**: [Arch Wiki](https://wiki.archlinux.org/)
- **Forum**: [Arch Linux Forums](https://bbs.archlinux.org/)

---

**Note**: This is unofficial software. Always keep your system updated and understand what packages you're installing.
