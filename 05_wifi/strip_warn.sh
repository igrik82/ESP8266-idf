#!/usr/bin/env bash
sed -i -E 's#-fstrict-volatile-bitfields##g' compile_commands.json
sed -i -E 's#-mlongcalls##g' compile_commands.json
