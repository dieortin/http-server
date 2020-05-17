import sys
import signal

TIMEOUT = 1 # seconds
signal.signal(signal.SIGALRM, input)
signal.alarm(TIMEOUT)

print("Inicio")
print("Script Python Conversor\n")


print("Recibido por STDIN: ")
try:
	for line in sys.stdin:
		argument = line.split('=')
		kel = int(argument[1]) + 273 #take the second part of the argument
		print(kel)
except:
    ignorar = True
print("Fin de datos")


print("\n\nRecibido por ARGV:")
argument = sys.argv[1].split('=')
kel = int(argument[1]) + 273 #take the second part of the argument
print(kel)
print("Fin de datos")


print("\n\nFin del script")
