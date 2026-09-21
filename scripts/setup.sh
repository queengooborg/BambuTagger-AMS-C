#!/bin/sh

set -eu

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repo_root"
git config core.hooksPath .githooks
printf '%s\n' 'Git hooks enabled from .githooks.'

if [ -d .venv ]; then
	printf '%s\n' 'Virtual environment already exists at .venv.'
else
	python_command=python3.13
	if ! command -v "$python_command" >/dev/null 2>&1; then
		python_command=python3.12
	fi
	if ! command -v "$python_command" >/dev/null 2>&1; then
		python_command=python3.11
	fi
	if ! command -v "$python_command" >/dev/null 2>&1; then
		printf '%s\n' 'Python 3.11-3.13 is required to create a virtual environment.' >&2
		exit 1
	fi
	python_path=$(command -v "$python_command")

	if command -v mkvenv >/dev/null 2>&1; then
		printf 'Creating a virtual environment with mkvenv using %s.\n' "$python_path"
		mkvenv -p "$python_path"
	elif command -v zsh >/dev/null 2>&1 && zsh -ic 'command -v mkvenv >/dev/null 2>&1' >/dev/null 2>&1; then
		printf 'Creating a virtual environment with mkvenv using %s.\n' "$python_path"
		zsh -ic 'cd "$1" && mkvenv -p "$2"' setup "$repo_root" "$python_path"
	else
		printf 'Creating .venv with %s.\n' "$python_command"
		"$python_command" -m venv .venv
	fi

	printf "Installing Python requirements.\n"
	"$python_command" -m pip install -r rquirements.txt
fi