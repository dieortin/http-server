import sys
import signal
TIMEOUT = 1 # seconds
signal.signal(signal.SIGALRM, input)
signal.alarm(TIMEOUT)

print("Inicio")
print("Script Python Nombre\n")


print("Recibido por STDIN: ")
try:
    print("Hola " + sys.stdin[1] + "!")
except:
    ignorar = True
print("Fin de datos")


print("\n\nRecibido por ARGV:")
print("Hola " + sys.argv[1] + "!")
print("Fin de datos")


print("\n\nFin del script")
