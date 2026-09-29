#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

TEST_FILE="$SCRIPT_DIR/demo_file.txt"
OUTPUT_FILE="$SCRIPT_DIR/demo_output.txt"
EXPECTED_FILE="$SCRIPT_DIR/expected_demo.txt"

echo "========================================"
echo " PRUEBAS DEL EDITOR DE TEXTO"
echo "========================================"

echo
echo "[1] Limpiando pruebas anteriores..."

rm -f "$TEST_FILE" "$OUTPUT_FILE" "$EXPECTED_FILE"

echo
echo "[2] Compilando proyecto..."

if ! make -C "$PROJECT_DIR"; then
    echo "ERROR: El proyecto no compilo correctamente."
    exit 1
fi

echo
echo "[3] Ejecutando comandos del editor..."

"$PROJECT_DIR/editor" > "$OUTPUT_FILE" 2>&1 <<EOF
o $TEST_FILE
a uno
a dos
a tres
p
p 2
i 1 inicio
i 5 final
d 3
s tres
p
p 99
d 99
i 99 imposible
s inexistente
comando_invalido
q
EOF

echo
echo "Salida producida por el editor:"
echo "----------------------------------------"
cat "$OUTPUT_FILE"
echo "----------------------------------------"

printf "inicio\nuno\ntres\nfinal\n" > "$EXPECTED_FILE"

echo
echo "[4] Verificando contenido final..."

if cmp -s "$TEST_FILE" "$EXPECTED_FILE"; then
    echo "OK: El contenido final del archivo es correcto."
else
    echo "ERROR: El contenido final no coincide con lo esperado."
    echo
    echo "Contenido esperado:"
    cat "$EXPECTED_FILE"
    echo
    echo "Contenido obtenido:"
    cat "$TEST_FILE"
    exit 1
fi

echo
echo "[5] Verificando casos borde..."

ERRORS=0

if grep -Fq "La linea 99 no existe." "$OUTPUT_FILE"; then
    echo "OK: Se detecto p sobre una linea inexistente."
else
    echo "ERROR: No se detecto correctamente p 99."
    ERRORS=$((ERRORS + 1))
fi

if grep -Fq "No se puede insertar en la linea 99." "$OUTPUT_FILE"; then
    echo "OK: Se detecto insercion en linea inexistente."
else
    echo "ERROR: No se detecto correctamente i 99."
    ERRORS=$((ERRORS + 1))
fi

if grep -Fq "No se encontraron coincidencias para: inexistente" "$OUTPUT_FILE"; then
    echo "OK: Se manejo correctamente una busqueda sin resultados."
else
    echo "ERROR: Fallo la prueba de busqueda sin resultados."
    ERRORS=$((ERRORS + 1))
fi

if grep -Fq "Comando no reconocido: comando_invalido" "$OUTPUT_FILE"; then
    echo "OK: Se detecto un comando incorrecto."
else
    echo "ERROR: No se detecto correctamente el comando incorrecto."
    ERRORS=$((ERRORS + 1))
fi

echo

if [ "$ERRORS" -eq 0 ]; then
    echo "========================================"
    echo " TODAS LAS PRUEBAS TERMINARON CORRECTAMENTE"
    echo "========================================"
    exit 0
else
    echo "========================================"
    echo " SE ENCONTRARON $ERRORS ERROR(ES)"
    echo "========================================"
    exit 1
fi
