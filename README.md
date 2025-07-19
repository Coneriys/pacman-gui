# Pacman GUI

A simple and intuitive GTK4 graphical interface for pacman package manager with AUR support for Arch Linux.

## Features

- 🔍 **Search packages** in official repositories and AUR
- 📦 **Install/Remove packages** with real-time logs
- 🔄 **System updates** with progress tracking
- 🌙 **Dark theme support** - follows system theme automatically
- 🛠️ **AUR support** via yay or paru
- 📋 **Live installation logs** in separate window
- 🎯 **Beginner-friendly** interface

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

1. **Search packages**: Enter package name and click Search
2. **Choose source**: Select "Official Repos" or "AUR" from dropdown
3. **Install**: Select package from list and click Install
4. **Remove**: Select installed package and click Remove
5. **Update system**: Click "Update System" button

### AUR Support

The application automatically detects installed AUR helpers (yay/paru). If none found, AUR search will be disabled.

## Architecture

```
src/
├── main.c              # Application entry point
├── pacman_wrapper.c    # Package manager backend
├── pacman_wrapper.h    # Backend interface
└── ui/
    ├── main_window.c   # Main GUI implementation
    └── main_window.h   # GUI interface
```

### Key Components

- **pacman_wrapper**: Handles pacman/AUR operations asynchronously
- **main_window**: GTK4 interface with real-time log display
- **Async operations**: Non-blocking package operations with live output

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

## Roadmap

- [ ] Package dependency visualization
- [ ] Installed packages management
- [ ] Package categories/groups
- [ ] Configuration settings dialog
- [ ] Repository management
- [ ] Package cache cleanup
- [ ] Multiple language support

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
