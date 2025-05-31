# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- `requirements.txt` with Flask, FastAPI, pandas, Jinja2, etc.
- Dashboard customization instructions in `README.md`
- Centralized path-safe logging using `log_file` in both Flask and FastAPI apps
- `dashboard.html`, `style.css`, and `chart.min.js` separation into proper `templates/` and `static/` folders

### Changed
- Refactored Flask app to use `template_folder` and `static_folder` args
- Refactored FastAPI app to use `Path()` for static/template directories and sensor log path
- Cleaned and structured `main.cpp` to remove credentials from Git history

### Fixed
- Corrected static file mount typo (`" static"` → `"static"`) in FastAPI
- Ensured consistent chart data formatting and error handling in historical routes

## [0.1.0] - 2025-05-30
Initial version with basic sensor reading, HTTP API, and CSV logging
