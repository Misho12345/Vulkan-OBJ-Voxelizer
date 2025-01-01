#!/bin/bash

SHADER_DIR="$(dirname "$(realpath "$0")")/src"
OUTPUT_DIR="$(dirname "$(realpath "$0")")/spv"
TIMESTAMP_FILE="/tmp/boza_shader_timestamps.txt"

mkdir -p "$OUTPUT_DIR"

TEMP_TIMESTAMP_FILE="/tmp/shader_timestamps_current.txt"
> "$TEMP_TIMESTAMP_FILE"

pushd "$SHADER_DIR" > /dev/null || exit

for f in *.vert *.frag *.geom *.comp *.tesc *.tese *.mesh *.task *.rgen *.rint *.rahit *.rchit *.rmiss; do
    if [ -f "$f" ]; then
        shader_time=$(stat -c %y "$f")
        echo "$f $shader_time" >> "$TEMP_TIMESTAMP_FILE"
    fi
done

popd > /dev/null || exit

if ! diff "$TEMP_TIMESTAMP_FILE" "$TIMESTAMP_FILE" > /dev/null; then
    pushd "$SHADER_DIR" > /dev/null || exit

    for f in *.vert *.frag *.geom *.comp *.tesc *.tese *.mesh *.task *.rgen *.rint *.rahit *.rchit *.rmiss; do
        if [ -f "$f" ]; then
            output_file="$OUTPUT_DIR/${f}.spv"

            if [ ! -f "$output_file" ]; then
                echo "Compiling new shader: $f"
                glslc -I"$SHADER_DIR" "$f" -o "$output_file"
            else
                shader_time=$(stat -c %y "$f")
                output_time=$(stat -c %y "$output_file")
                if [ "$shader_time" -ne "$output_time" ]; then
                    echo "Recompiling changed shader: $f"
                    glslc -I"$SHADER_DIR" "$f" -o "$output_file"
                fi
            fi
        fi
    done

    popd > /dev/null || exit

    mv "$TEMP_TIMESTAMP_FILE" "$TIMESTAMP_FILE"
else
    rm "$TEMP_TIMESTAMP_FILE"
fi
