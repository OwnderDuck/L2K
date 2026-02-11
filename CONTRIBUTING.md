# Contributing to L2K

Thank you for helping L2K! Fork the repo and submit a PR.

## 🌍 i18n (Translations)

L2K automatically detects system language via the `$LANG` environment variable. To add a new language:

1. **Copy**: Open `i18n.h` and copy any existing `LanguagePack` (e.g., `en_US`).
2. **Rename**: Rename the variable to your target language code (e.g., `ja_JP`).
3. **Translate**: Translate the strings into your language.
4. **The MYY Rule**: If you see `MYY`, **keep it exactly as-is**. It is a placeholder for unused/internal fields. Do not translate or change them.
5. **Register (Optional)**: If you know how, add the detection logic to `init_i18n()` in `l2k.cpp`. 
   > **Note**: If you are not comfortable editing `l2k.cpp`, **just leave it to me!** Submit your PR with the new `i18n.h` changes, and I will handle the registration logic for you.

## 🛠 Development Workflow

- **Requirements**: C++17 compiler, Linux kernel.
- **Build**:
```bash
g++ --std=c++17 l2k.cpp -o l2k
```
