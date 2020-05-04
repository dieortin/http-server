import sys
import signal
import urllib.parse

TIMEOUT = 1 # seconds
signal.signal(signal.SIGALRM, input)
signal.alarm(TIMEOUT)

print("Inicio")
print("Script Python Nombre\n")

print("Recibido por STDIN: ")
try:
	dic = urllib.parse.parse_qs(sys.stdin[1])
	print("Hola " + dic['var'][0] + "!")
except:
    ignorar = True
print("Fin de datos")


print("\n\nRecibido por ARGV:")
dic = urllib.parse.parse_qs(sys.argv[1])
print("Hola " + dic['var'][0] + "!")
print("Fin de datos")


print("\n\nFin del script")
