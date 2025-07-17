# Contributing to QuestAdbLib

Thank you for your interest in contributing to QuestAdbLib! This document provides guidelines for contributing to the project.

## Development Setup

1. **Fork and Clone**
   ```bash
   git clone https://github.com/yourusername/QuestAdbLib.git
   cd QuestAdbLib
   ```

2. **Build the Project**
   ```bash
   # Windows
   ./build.bat
   
   # Unix/Linux/macOS
   ./build.sh
   ```

3. **Run Examples**
   ```bash
   # After building, examples are in build/examples/
   ./build/examples/simple_example
   ```

## Code Style

- Follow the existing code style in the project
- Use the provided `.clang-format` configuration
- Run clang-format before submitting:
  ```bash
  find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i
  ```

## Testing

- Ensure all examples compile and run successfully
- Test on multiple platforms if possible (Windows, macOS, Linux)
- Verify that ADB operations work with actual Quest devices

## Submitting Changes

1. **Create a Branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make Your Changes**
   - Follow the code style guidelines
   - Add appropriate error handling
   - Update documentation if needed

3. **Test Your Changes**
   - Build the project successfully
   - Run the examples
   - Test with real Quest devices if available

4. **Commit Your Changes**
   ```bash
   git add .
   git commit -m "Add: brief description of your changes"
   ```

5. **Push and Create PR**
   ```bash
   git push origin feature/your-feature-name
   ```
   Then create a Pull Request on GitHub.

## Pull Request Guidelines

- **Title**: Use a clear, descriptive title
- **Description**: Explain what changes you made and why
- **Testing**: Describe how you tested your changes
- **Breaking Changes**: Clearly mark any breaking changes

## Code Review Process

1. All PRs must pass the CI pipeline
2. Code will be reviewed for:
   - Correctness and functionality
   - Code style and consistency
   - Performance implications
   - Thread safety considerations
   - Error handling

## Areas for Contribution

- **Bug Fixes**: Fix issues with device detection, configuration, or metrics
- **Platform Support**: Improve cross-platform compatibility
- **Performance**: Optimize ADB command execution or device monitoring
- **Documentation**: Improve examples, API documentation, or guides
- **Testing**: Add automated tests or improve test coverage

## Questions?

If you have questions about contributing, please:
1. Check the existing issues on GitHub
2. Look at the `CLAUDE.md` file for architecture details
3. Create a new issue for discussion

Thank you for contributing to QuestAdbLib!