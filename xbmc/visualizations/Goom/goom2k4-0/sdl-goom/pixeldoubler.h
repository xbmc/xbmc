#ifndef _PIXELDOUBLER_H
#define _PIXELDOUBLER_H

#include "surface.h"

/*
 * copie la surface src dans la surface dest en doublant la taille des pixels.
 *
 * la surface dest doit faire exactement 2 fois la taille de la surface src.
 * (segfault sinon).
 *
 * auteur : JeKo <jeko@free.fr>
 *
 * bench : <2001-11-28|20h00> 9 cycles par pixel marqué (cpm) sur un PII 266.
 *           (surement limité par le debit de la memoire vive..
 *            je fonce chez moi verifier)
 *         <chez moi|1h10> 11 cpm sur un Duron 800. (i.e. pas loin de 300fps)
 *           surement que les acces memoires sont assez penalisant.
 *           je tente d'aligner les données des surfaces pour voir.
 *           => pas mieux : le systeme doit deja faire ca comme il faut.
 *           mais pour l'alignement 64bits ca va etre utile : je passe a l'ASM
 *         <3h00> l'optimisation asm a permi de gagner vraiment pas grand
 *           chose (0.1 ms sur mon Duron).. le code en C semble suffisant.
 *           et je persiste a croire ke la vitesse est plafonné par la vitesse
 *           d'acces a la memoire.. ceci expliquerait aussi cela.
 *
 *         <2001-12-08|1h20> Travail sur le code assembleur :
 *           pour reduire les temps d'acces memoire, utilisation de
 *           l'instruction 3Dnow! PREFETCH/W pour le prechargement des
 *           page de cache. pour pousser cette optimisation jusque au bout :
 *           j'ai déroulé la boucle pour qu'elle traite a chaque passage
 *           une page de cache complete en lecture et 2 en ecriture.
 *           (optimisé sur un Duron=Athlon, page de cache = 64 octets)
 *           preformances sur mon Duron 800 : 9 CPM.
 *           (ce qui fait 18% de mieux que la version en C)
 *           ATTENTION : CETTE VERSION NE SUPPORTE DONC QUE DES TAILLES DE
 *           SURFACE AYANT UNE LARGEUR MULTIPLE DE 32 POUR DEST,
 *           DONC 16 POUR SRC. (ce qui n'est pas tres genant puisque ce sont
 *           des resolutions standard, mais il faut le savoir)
 *           explication : alignement des données sur la taille des pages de
 *           cache.
 *           
 *         <2001-12-08|14h20> Apres intense potassage de la doc de l'Athlon,
 *           decouverte certaines subtilités de ce FABULEUX processeur :)
 *           entrelacement de la copie de 2 pixel, plus utilisation de
 *           l'instruction de transfert rapide 3Dnow! MOVNTQ... attention les
 *           chiffres -> sur mon Duron 800 : 4 CPM !!!!!
 *
 * note : ne fonctionne que sur un systeme 32bits.. mais le faire fonctionner
 *        sur une machine autre ne posera aucun probleme.
 *        (le seul truc c'est ke j'ai considéré mes pointeurs comme des entiers
 *        32bits <- je sais je suis vaxiste, et alors???:)
 *
 * copyright (c)2001, JC Hoelt for iOS software.
 */
void pixel_doubler (Surface *src, Surface *dest) ;

#endif
