# Affiche une barre de progression
# Arguments : (y) (titre) (valeur)
#	y : ligne du terminal à partir de la quelle sera affichée la barre
#	titre : Titre affiché au dessus de la barre
#	valeur : valeur à afficher comprise entre 255 et 0
function AfficheBarre () 
{
	# Calcule la largeur effective de la barre à l'écran
	largeur=$(($(($3 * $LargeurEcran)) / 255 ))
	
	#Positionne le curseur
	tput cup $1 0
	
	#Affiche le titre
	echo "$2 : $3     "
	
	# Affiche la partie pleinne de la barre
	for i in $(seq 1 $largeur); do
		echo -n "#"
	done
	
	#Affiche la partie vide de la barre (sert à effacer l'ancienne position)
	for i in $(seq $largeur $(($LargeurEcran-1))); do
		echo -n " "
	done

}

# Affiche l'état des bits d'une valeurs numériques (5 bits affichés)
# Arguments : (valeur)
#	valeur : valeur à afficher 
function AfficheValeurNumeric () {

	d=$1

	echo "Numeric"
	
	if [ "$d" -ge 16 ]; then
		echo -n "[#]"
		d=$(($d-16))
	else
		echo -n "[ ]"
	fi
	
	if [ "$d" -ge 8 ]; then
		echo -n "[#]"
		d=$(($d-8))
	else
		echo -n "[ ]"
	fi
	
	if [ "$d" -ge 4 ]; then
		echo -n "[#]"
		d=$(($d-4))
	else
		echo -n "[ ]"
	fi
	
	if [ "$d" -ge 2 ]; then
		echo -n "[#]"
		d=$(($d-2))
	else
		echo -n "[ ]"
	fi
	
	if [ "$d" -ge 1 ]; then
		echo -n "[#]"
		d=$(($d-1))
	else
		echo -n "[ ]"
	fi

}

# Boucle générale du programme
function Boucle () 
{
	while true ; do

		# Lit les données grace au programme
		donnes=$(./k8055)
		
		# Sépare les valeurs en utilisant cut
		d=$(echo $donnes | cut -f2 -d";")
		a1=$(echo $donnes | cut -f3 -d";")
		a2=$(echo $donnes | cut -f4 -d";")
		c1=$(echo $donnes | cut -f5 -d";")
		c2=$(echo $donnes | cut -f6 -d";")
		
		# Affiche la barre pour l'entrée analogique 1
		AfficheBarre 4 "Analog 1" "$a1"
		
		# Affiche la barre pour l'entrée analogique 2
		AfficheBarre 6 "Analog 2" "$a2"
		
		# Affiche l'état des compteurs
		echo "Counter 1 : $c1"
		echo "Counter 2 : $c2"
		
		# Affiche l'état de l'entrée numérique
		AfficheValeurNumeric $d
		
		# Lit l'entrée clavier
		read
		case $REPLY in				
			q) return # Quitte le programme si l'entrée est "q"
			;;
			*)
			;;
		esac

	done
}

# Intilialise le terminal pour le programme
function initialiser () 
{

	# Mémorise la largeur du terminal
	LargeurEcran=$(tput cols)

	# Initialise
	if !tput init ; then
		echo "Impossible d'initialiser le terminal"
		exit 1
	fi

	# Defini le mode de saisie sur l'entrée standart : temps d'attente = 0sec, 0 caractères minimum à lire
	stty -icanon -echo time 0 min 0	
	# Nettoie le terminal
	tput clear
	# Cache le curseur
	tput civis
}

# Restaure les paramétres par défaut du terminal
function deinitialiser () 
{
	# Remet les paramètres par défaut de l'affichage du terminal
	tput reset
	# Remet les paramètres par défaut de saisie du terminal
	stty icanon echo 
	# Nettoie le terminal
	clear
}

initialiser

Boucle

deinitialiser
