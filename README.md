# KeyGhost
## A Secure Text Snippet Manager with Hotkey Support

KeyGhost is a utility that securely stores and types your sensitive text like passwords, frequently used phrases, or code snippets. With customizable hotkeys, you can quickly insert text anywhere without using clipboard or manual typing.

> **⚠️ IMPORTANT NOTICE:** This is an alpha version of KeyGhost that may contain bugs and stability issues. Use at your own risk and please report any issues you encounter to help improve the application.

![KeyGhost Screenshot](https://github.com/user-attachments/assets/914c12b8-2510-403f-b78f-d3173d9d0b54)

## Features

- **Secure Text Storage**: All snippets are encrypted before being stored
- **Hotkey Integration**: Assign keyboard shortcuts to each text snippet
- **Automatic Typing**: Simulates keyboard input or uses clipboard
- **Security Options**:
  - Text masking for sensitive data
  - Auto-clearing after use
  - Clipboard security features
- **System Tray Access**: Quick access to your snippets from the system tray

## Usage Examples

### Simplifying noVNC Password Entry
One common frustration when using noVNC is entering complex passwords, especially with different keyboard layouts:

1. Create a new snippet in KeyGhost named "VNC Password"
2. Enter your VNC password in the text field
3. Assign an easy-to-remember hotkey like Alt+V
4. When connecting to noVNC:
   - Click on the password field
   - Press Alt+V
   - KeyGhost automatically types your password correctly, regardless of keyboard layout

This eliminates typos, handles special characters automatically, and keeps your password secure without exposing it in the clipboard.

## License

KeyGhost is released under the GNU Lesser General Public License (LGPL-3.0). Please refer to the [LICENSE](https://github.com/mxrcode/KeyGhost/blob/main/LICENSE) file for more details.

## Contributing

Contributions are welcome! Feel free to submit pull requests or report issues on the GitHub repository.

## Security

KeyGhost implements encryption for stored snippets and includes various security features to protect sensitive data. However, no software is entirely secure, and caution should be exercised when storing sensitive information.

## Disclaimer

KeyGhost is provided "as is" without any warranty. The developers are not responsible for any loss or damage resulting from the use of this application.

## Contact

For questions, feedback, or issues, please contact the project maintainers via the GitHub repository.

---

Thank you for using KeyGhost! We hope it simplifies your workflow while keeping your sensitive information secure.
