import sys
import signal
TIMEOUT = 1 # seconds
signal.signal(signal.SIGALRM, input)
signal.alarm(TIMEOUT)

print("Inicio")
print("Script Python Conversor\n")


print("Recibido por STDIN: ")
try:
	kel = sys.stdin[1] + 273
    print(kel)
except:
    ignorar = True
print("Fin de datos")


print("\n\nRecibido por ARGV:")
kel = sys.argv[1] + 273
print(kel)
print("Fin de datos")


print("\n\nFin del script")
