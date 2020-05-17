import sys
import signal

TIMEOUT = 1 # seconds
signal.signal(signal.SIGALRM, input)
signal.alarm(TIMEOUT)

print("Inicio")
print("Script Python Nombre\n")

print("Recibido por STDIN: ")
try:
	for line in sys.stdin:
		argument = line.split('=')
		argument_res = argument[1].split("\r")
		print("Hola " + argument_res[0] + "!")	#take the second part of the argument
except:
    ignorar = True
print("Fin de datos")


print("\n\nRecibido por ARGV:")
argument = sys.argv[1].split('=')
print("Hola " + argument[1] + "!") #take the second part of the argument
print("Fin de datos")


print("\n\nFin del script")
