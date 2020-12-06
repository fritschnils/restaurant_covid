#ifndef SHM_H
#include "shm.h"

void usage()
{
    fprintf(stderr, "usage: restaurant <duree> <table_sizes>...\n");
    
    if(fflush(stderr) == EOF)
        raler("fflush", 1);
}


/** Création d'un segment de mémoire partagée contenant un restaurant.

    Créer un nouveau segment de mémoire partagée de nom #NOM_RESTAURANT 
    (qui ne doit pas déjà exister).

    Initialise le contenu du segment c'est à dire :
        - Les variables : taille et ouvert
        - Les sémaphores
        - La structure de requête de convive
        - Le tableau de communication

    \param nb_table Le nombre de tables
    \returns L'adresse du nouveau segment, projeté en mémoire
*/
struct restaurant *creation_restaurant(int nb_table)
{   
    int fd, i;
    struct restaurant *m_restaurant;
    ssize_t octets_rest = SIZE_RESTAURANT(nb_table);

    if ((fd = shm_open(NOM_RESTAURANT
            , O_RDWR| O_CREAT| O_EXCL, 0666)) == -1)
        raler("shm_open", 1);

    if (ftruncate(fd, octets_rest) == -1)
        raler("ftruncate", 1);

    if ((m_restaurant = mmap(NULL, octets_rest, PROT_READ | PROT_WRITE, 
            MAP_SHARED, fd, 0)) == MAP_FAILED)
        raler("mmap", 1);

    // Initialiser variables de la struct restaurant 
    m_restaurant -> ouvert = 0;
    m_restaurant -> taille = octets_rest;
    
    if (sem_init(&m_restaurant -> crit_ouvert, 1, 1) == -1)
        raler("sem_init crit_ouvert", 1);

    if (sem_init(&m_restaurant -> serveur_dispo, 1, 0) == -1)
        raler("sem_init serveur_dispo", 1);

    if (sem_init(&m_restaurant -> requete_ecrite, 1, 0) == -1)
        raler("sem_init requete_ecrite", 1);

    if (sem_init(&m_restaurant -> reponse_serveur, 1, 0) == -1)
        raler("sem_init reponse_serveur", 1);

    if (sem_init(&m_restaurant -> ack_convive, 1, 0) == -1)
        raler("sem_init ack_convive", 1);

    if (sem_init(&m_restaurant -> besoin_nettoyage, 1, 0) == -1)
        raler("sem_init besoin_nettoyage", 1);

    if (sem_init(&m_restaurant -> police_presente, 1, 0) == -1)
        raler("sem_init police_presente", 1);

    if (sem_init(&m_restaurant -> compte_rendu_pret, 1, 0) == -1)
        raler("sem_init compte_rendu_pret", 1);

    if (sem_init(&m_restaurant -> couvre_feu, 1, 0) == -1)
        raler("sem_init couvre_feu", 1);

    m_restaurant -> req_conv.nom_convive[0] = '\0'; 
    m_restaurant -> req_conv.nom_chef[0] = '\0';
    m_restaurant -> req_conv.taille_grp = -1;

    for (i = 0; i < nb_table; i++)
        if (sem_init(&m_restaurant -> fin_repas[i], 1, 0) == -1)
            raler("sem_init fin_repas", 1);

    return m_restaurant;
}


/** Suppression du segment de mémoire partagée.

    Tous les sémaphores contenus dans le segment sont détruits à
    l'aide de sem_destroy(). La projection est ensuite supprimée.
    Enfin, le nom du segment (#NOM_RESTAURANT) est passé à shm_unlink().
    Le segment sera détruit lorsque le dernier processus qui le projette 
    dans son espace d'adressage aura appelé #stock_unmap.
    Normalement, ce sera le processus de restaurant.

    \param restaurant Un pointeur vers le début de la projection.
    \returns Cette fonction ne renvoie rien.
*/
void destruction_restaurant(struct restaurant *restaurant, int nb_table)
{
    int i;

    if (sem_wait(&restaurant -> crit_ouvert) == -1)
        raler("sem_wait crit_ouvert", 1);

    if (restaurant -> ouvert)
    {
        if (sem_post(&restaurant -> crit_ouvert) == -1)
            raler("sem_wait crit_ouvert", 1);
        
        raler("destruction alors que restaurant ouvert", 0);
    }

    // Destruction des sémaphores
    if (sem_destroy(&restaurant -> crit_ouvert) == -1)
        raler("destroy crit_ouvert", 1);

    if (sem_destroy(&restaurant -> serveur_dispo) == -1)
        raler("destroy serveur_dispo", 1);

    if (sem_destroy(&restaurant -> requete_ecrite) == -1)
        raler("destroy requete_ecrite", 1);

    if (sem_destroy(&restaurant -> reponse_serveur) == -1)
        raler("destroy reponse_serveur", 1);

    if (sem_destroy(&restaurant -> ack_convive) == -1)
        raler("destroy ack_convive", 1);

    if (sem_destroy(&restaurant -> police_presente) == -1)
        raler("destroy police_presente", 1);

    if (sem_destroy(&restaurant -> besoin_nettoyage) == -1)
        raler("destroy besoin_nettoyage", 1);

    if (sem_destroy(&restaurant -> compte_rendu_pret) == -1)
        raler("destroy police_presente", 1);

    if (sem_destroy(&restaurant -> couvre_feu) == -1)
        raler("destroy compte_rendu_pret", 1);

    for (i = 0; i < nb_table ; i++)
        if (sem_destroy(&restaurant -> fin_repas[i]) == -1)
            raler("sem_destroy fin_repas", 1);

    if(munmap(restaurant, restaurant -> taille) == -1)
        raler("munmap", 1);

    if(shm_unlink(NOM_RESTAURANT) == -1)
        raler("shm_unlink", 1);
}


/** Ouvre ou ferme le restaurant.

    Cette fonction ouvre ou ferme le restaurant selon le mode.
    Si mode == 1 c'est une ouverture, sinon c'est une fermeture.

    \param mode Mode d'utilisation de la fonction.
    \returns Cette fonction ne renvoie rien.
*/
void ouverture_fermeture_restaurant(int mode)
{
    struct restaurant *restaurant = restaurant_map();

    if (sem_wait(&restaurant -> crit_ouvert) == -1)
        raler("sem_wait critique", 1);

    if ((mode && restaurant -> ouvert) || (!mode &&  !restaurant -> ouvert))
    {
        if (sem_post(&restaurant -> crit_ouvert) == -1)
            raler("sem_wait crit_ouvert", 1);
        
        raler("ouverture/fermeture mais restaurant est deja ouvert/fermé", 0);
    }

    if (mode)
    {
        restaurant -> ouvert = 1;
        if (sem_post(&restaurant -> serveur_dispo) == -1)
            raler("sem_post serveur_dispo", 1);
    }

    else
        restaurant -> ouvert = 0;

    if (sem_post(&restaurant -> crit_ouvert) == -1)
        raler("sem_wait critique", 1);

    restaurant_unmap(restaurant);
}


/** Ajustement de la structure timespec

    Après cette fonction, les valeurs de la structure de temps sont ajustées.
    Si il y a trop de ns, on convertit en seconde.
    Si il y a pas assez de ns on convertit une seconde en ns.

    \param t La structure de temps à ajuster
    \param n Durée du service en ns
    \returns Cette fonction ne renvoie une nouvelle structure ajustée.
*/
struct timespec ajuster_tps(struct timespec t, int n)
{
    t.tv_sec += (n/1000000000);
    t.tv_nsec += (n%1000000000);

    if(t.tv_nsec >= 1000000000)
    {
        t.tv_sec++;
        t.tv_nsec -= 1000000000;
    }
    else if (t.tv_nsec < 0)
    {
        t.tv_sec --;
        t.tv_nsec += 1000000000;
    }

    return t;
}


/** Lancement d'un repas chronometré

    Cette fonction éxécutée par un processus fils du restaurant lance
    un repas. Quand le repas arrive à sa fin, libère le sémaphore de la
    table autant de fois que de convives attendent.

    \param duree_service La durée du repas en ns
    \param indice L'indice de la table du repas
    \param nb_conv Le nombre de convives à cette table
    \returns Cette fonction ne renvoie rien.
*/
void lancer_chrono(int duree_service, int indice, int nb_conv)
{
    int i;
    sem_t faux_sem;
    struct timespec time;
    struct restaurant *restaurant = restaurant_map();

    if (sem_init(&faux_sem, 0, 0) == -1)
        raler("sem_init faux_sem", 1);
    
    if (clock_gettime(CLOCK_REALTIME, &time) == -1)
        raler("clock_gettime", 1);

    time = ajuster_tps(time, duree_service * 1000000);

    if (sem_timedwait(&faux_sem, &time) == -1)
        if (errno != ETIMEDOUT)
            raler("timedwait", 1);

    for(i = 0; i < nb_conv; i++)
    {
        if (sem_post(&restaurant -> fin_repas[indice]) == -1)
            raler("sem_post fin_repas", 1);
    }
    
    if (sem_post(&restaurant -> besoin_nettoyage) == -1)
        raler("sem_post besoin_nettoyage", 1);

    if (sem_destroy(&faux_sem) == -1)
        raler("sem_destroy faux_sem", 1);

    restaurant_unmap(restaurant);

    exit(EXIT_SUCCESS);
}


/** Affichage de l'état actuel de la salle

    Cette fonction sert uniquement à debuger.

    \param salle La salle à afficher
    \param nb_table Le nombre de tables de la salle
    \returns Cette fonction ne renvoie rien.
*/
void affiche_salle(struct table *salle, int nb_table)
{
    int i, j;
    printf("---------------------------------------------------------\n");
    for (i = 0; i < nb_table; i++)
    {
        printf("Table %d, Taille : %d, Nb conv : %d\n", i
                , salle[i].taille, salle[i].nb_conv);
        for(j = 0; j < salle[i].taille; j++)
        {
            printf(" %s|", salle[i].liste_conv[j]);
        }
        printf("\n");
    }
    printf("---------------------------------------------------------\n");
}


/** Attend la terminaison/changement d'état de n processus

    Cette fonction attend n fois un processus. 
    C'est simplement n fois la fonction wait avec les vérifications
    du code de retour

    \param n Le nombre de processus à récupérer
    \returns Cette fonction ne renvoie rien.
*/
void n_wait(int n)
{
    int reason;
    int j;

    for(j = 0; j < n; j++){
        if(wait(&reason) == -1)
            raler("wait", 1);
        if(!WIFEXITED(reason) || WEXITSTATUS(reason) == EXIT_FAILURE)
            raler("child terminated abnormally", 1);
    }
}


/** Transmission du compte rendu à la police

    1) Alloue le segment mémoire pour communication avec la police
    2) Présente le compte rendu (instantané + cahier de rappels)
    3) Supprime le segment mémoire

    \param salle La salle avec laquelle produire l'instantané
    \param cr Le cahier de rappels à fournir
    \param nb_tab Le nombre de tables de la salle
    \returns Cette fonction ne renvoie rien.
*/
void transmet_police(struct table *salle, struct cahier_rappel *cr, int nb_tab)
{
    int i, j, fd, nb_elem = nb_tab + cr -> nb_grp;
    ssize_t octets_cr = SIZE_COMPTE_RENDU(nb_elem);
    struct element *tmp = malloc(sizeof(struct element*));
    struct compte_rendu_shm *cr_shm;
    struct restaurant *restaurant = restaurant_map();

    tmp = cr -> head;

    // Allocations
    if ((fd = shm_open(NOM_COMPTE_RENDU
            , O_RDWR| O_CREAT| O_EXCL, 0666)) == -1)
        raler("shm_open", 1);

    if (ftruncate(fd, octets_cr) == -1)
        raler("ftruncate", 1);

    if ((cr_shm = mmap(NULL, octets_cr, PROT_READ | PROT_WRITE, 
            MAP_SHARED, fd, 0)) == MAP_FAILED)
        raler("mmap", 1);

    if (sem_init(&cr_shm -> ack_police, 1, 0) == -1)
        raler("sem_init ack_police", 1);

    //Ecriture compte rendu à la police
    cr_shm -> taille = octets_cr;
    cr_shm -> nb_grp = cr -> nb_grp;
    cr_shm -> nb_table = nb_tab;

    for (i = 0; i < nb_tab; i++)
    {
        cr_shm -> liste_elements[i].type = 1;
        cr_shm -> liste_elements[i].nb_conv = salle[i].nb_conv;
        for (j = 0; j < salle[i].nb_conv; j++)
        {   
            strncpy(cr_shm -> liste_elements[i].noms[j]
                        , salle[i].liste_conv[j], 11);
        }
    }

    i = 0;
    while(tmp)
    {
        cr_shm -> liste_elements[nb_tab + i].type = 0;
        cr_shm -> liste_elements[nb_tab + i].nb_conv = tmp -> nb_conv;

        for (j = 0; j < tmp -> nb_conv; j++)
        {
            strncpy(cr_shm -> liste_elements[nb_tab + i].noms[j]
                        , tmp -> noms[j], 11);
        }
        i++;
        tmp = tmp -> next;
    }

    // Signale compte_rendu pret
    if (sem_post(&restaurant -> compte_rendu_pret) == -1)
        raler("sem_post compte_rendu_pret", 1);

    if (sem_wait(&cr_shm -> ack_police) == -1)
        raler("sem_wait ack_police", 1);

    // Désallocations
    restaurant_unmap(restaurant);

    if (sem_destroy(&cr_shm -> ack_police) == -1)
        raler("sem_destroy ack_police", 1);

    if (munmap(cr_shm, octets_cr) == -1)
        raler("munmap", 1);

    if  (shm_unlink(NOM_COMPTE_RENDU) == -1)
        raler("shm_unlink", 1);    
}


/** Lancement d'un nettoyage de table

    Cette fonction est appelée pour nettoyer les tables.

    \param salle La salle à nettoyer
    \param nb_table Le nombre de tables de la salle
    \param r Le restaurant pour vérifier si des convives sont à la table
    \returns Cette fonction ne renvoie rien.
*/
void nettoyage_tables(struct table *salle, int nb_table, struct restaurant *r)
{
    int i, test_val;

    for (i = 0; i < nb_table; i++)
    {
        if (sem_getvalue(&r -> fin_repas[i], &test_val) == -1)
            raler("sem_getvalue nettoyage", 1);

        if(salle[i].nb_conv != 0 
                && salle[i].nb_conv == salle[i].nb_conv_attendus
                && !(test_val < 0))
        {
            salle[i].nb_conv = 0;
            salle[i].nb_conv_attendus = 0;
        }
    }
}


/** Affichage de l'état actuel de du cahier de rappels

    Cette fonction sert uniquement à debuger.

    \param cr Le cahier de rappels à afficher
    \returns Cette fonction ne renvoie rien.
*/
void afficher_cahier(struct cahier_rappel *cr)
{
    struct element *tmp = malloc(sizeof(struct element*));

    tmp = cr -> head;

    while(tmp){
            printf("groupe (%d) : ", tmp -> nb_conv);
            for (int i = 0; i < tmp -> nb_conv; i++)
                printf(" %s", tmp -> noms[i]);
            printf("\n");
            tmp = tmp -> next;
        }
    free(tmp);
}


/** Initialiser le cahier de rappels

    Cette fonction initialise le cahier de rappels

    \param cr Le cahier de rappels à afficher
    \returns Pointeur à l'adresse du cahier initialisé
*/
struct cahier_rappel *init_cahier()
{
    struct cahier_rappel *cr= malloc(sizeof(struct cahier_rappel*));

    if (cr == NULL)
        raler("malloc init cahier", 1);

    cr -> nb_grp = 0;
    cr -> head = NULL;

    return cr;
}


/** Ajouter un élément au cahier

    Cette fonction sert à ajouter un groupe au cahier de rappels.
    Appelée lors du démarrage d'une table.

    \param cr Le cahier de rappels
    \param t La table à laquelle mange le groupe à ajouter au cahier
    \returns Cette fonction ne renvoie rien.
*/
void insert_cahier(struct cahier_rappel* cr, struct table t)
{   
    int i;
    struct element *new = malloc(sizeof(struct element*));

    new -> next = malloc(sizeof(struct element*));
    if(new == NULL || new -> next == NULL)
        raler("malloc insert cahier", 1);

    for(i = 0; i < t.nb_conv; i++)
    {
        strncpy(new -> noms[i], t.liste_conv[i], 11);
    }

    new -> nb_conv = t.nb_conv;
    new -> next = cr -> head;  
    cr -> head = new;
    cr -> nb_grp++;
}


/** Compte les tables disponibles

    Cette fonction est appelée quand un convive fait une requete.
    Si la fonction renvoie 0, on essaiera un nettoyage de table

    \param cr Le cahier de rappels
    \param t La table à laquelle mange le groupe à ajouter au cahier
    \returns Cette fonction ne renvoie rien.
*/
int cpt_dispo(struct table* salle, int taille_grp, int nb_table)
{
    int places = 0, i;
    for (i = 0; i < nb_table; i++)
    {
        if (salle[i].nb_conv == 0 && salle[i].taille >= taille_grp)
            places++;
    }
    return places;
    (void)salle;
}


int main(int argc, char *argv[])
{
        if(fflush(stdout) == EOF)
        raler("fflush", 1);
    int nb_table = 0, test_convive = 0, test_couvrefeu = 0, i, j, places;
    int tmp, demarrer_table, nb_groupes_servis = 0, nb_convives_servis = 0;
    int taille_tmp, refouler = 0;
    long int duree_service = 0, *table_sizes;
    
    char *endptr, *str;
    
    struct restaurant *m_rest;
    struct table *salle;
    struct cahier_rappel *cahier_rappel;
    struct element *elt_tmp;

    /**************************************************************************
     *                          CHECK DES ARGUMENTS                           *
     *************************************************************************/
    // Check sur nombre d'arguments
    if (argc < 3)
    {
        usage();
        return EXIT_FAILURE;
    }
    nb_table = argc - 2;

    // Durée de service
    str = argv[1];
    errno = 0;
    duree_service = strtol(str, &endptr, 10);

    if ((errno == ERANGE && (duree_service == LONG_MAX
            || duree_service == LONG_MIN)) 
            || (errno != 0 && duree_service == 0))
    {
        usage();
        perror("strtol");
        return EXIT_FAILURE;
    }

    // Pas d'entier trouvé || Entier non valide || Mauvaise valeur
    if (endptr == str || *endptr != '\0' || duree_service <= 0)
    { 
        usage();
        return EXIT_FAILURE;
    }

    // Tailles des tables (puis stockage dans un tableau pour plus tard)
    if ((table_sizes = malloc(nb_table * sizeof(long int))) == NULL)
        raler("malloc : nb tables", 0);

    for (i = 0; i < nb_table; i++)
    {
        str = argv[i+2];
        errno = 0;

        table_sizes[i] = strtol(str, &endptr, 10);  
        if ((errno == ERANGE && (table_sizes[i] == LONG_MAX
                || table_sizes[i] == LONG_MIN)) 
                || (errno != 0 && table_sizes[i] == 0))
        {
            usage();
            perror("strtol");
            return EXIT_FAILURE;
        }

        if (endptr == str || *endptr != '\0' || table_sizes[i] <= 0 
                || table_sizes[i] > 6)
        {
            usage();
            return EXIT_FAILURE;
        }
    }


    /**************************************************************************
     *                          INITIALISATIONS                               *
     *************************************************************************/ 
    // Création cahier rappel
    cahier_rappel = malloc(sizeof(struct cahier_rappel*));
    if (cahier_rappel == NULL)
        raler("malloc cahier rappel", 1);

    cahier_rappel = init_cahier();

    printf("allocation salle\n");
    // Allocation salle
    salle = malloc(nb_table * sizeof(struct table));
    if (salle == NULL)
        raler("malloc tables", 1);

    for (i = 0; i < nb_table; i++)
    {
        salle[i].taille = table_sizes[i];
        salle[i].nb_conv = 0;
        salle[i].nb_conv_attendus = 0;
        salle[i].liste_conv = malloc(salle[i].taille * sizeof(char*));
        if (salle[i].liste_conv == NULL)
            raler("malloc listes noms", 1);
        for (j = 0; j < salle[i].taille; j++)
        {
            salle[i].liste_conv[j] = malloc(11 * sizeof(char));
            if (salle[i].liste_conv[j]== NULL)
                raler("malloc noms", 1); 
            salle[i].liste_conv[j][0] = '\0'; 
        }
    }

    // Création restaurant puis ouverture
    m_rest = creation_restaurant(nb_table);
    ouverture_fermeture_restaurant(1);


    /**************************************************************************
     *                          DEBUT TRAVAILLE                               *
     *************************************************************************/
    while (!test_couvrefeu)
    {
        /**********************************************************************
         *                      TRAITE ARRIVEE CONVIVE                        *
         *********************************************************************/
        if (sem_getvalue(&m_rest -> serveur_dispo, &test_convive) == -1)
            raler("sem_getvalue", 1);

        if (test_convive < 1)
        {
            if (sem_post(&m_rest -> serveur_dispo) == -1)
                raler("sem_post serveur_dispo", 1);
        
            if (sem_wait(&m_rest -> requete_ecrite) == -1)
                raler("sem_wait requete_ecrite", 1);
            
            tmp = -1;
            taille_tmp = 7;
            demarrer_table = 0;

            places = cpt_dispo(salle, m_rest -> req_conv.taille_grp, nb_table);
            
            if (sem_trywait(&m_rest -> besoin_nettoyage) == -1)
            {
                if(errno != EAGAIN)
                    raler("sem_trywait", 1);
            }
            else
                nettoyage_tables(salle, nb_table, m_rest);

            // Si c'est un chef cherche la plus petite table qui va bien
            if ((m_rest -> req_conv.taille_grp != -1) && !refouler && places)
            {
                for (i = 0; i < nb_table; i++)
                {
                    if(salle[i].nb_conv == 0 
                            && table_sizes[i] >= m_rest -> req_conv.taille_grp
                            && table_sizes[i] < taille_tmp)
                    {
                        taille_tmp = table_sizes[i];
                        tmp = i;
                    }
                }

                if (tmp != -1)
                {
                    strncpy(salle[tmp].liste_conv[0]
                            , m_rest -> req_conv.nom_convive, 11);
                    salle[tmp].nb_conv ++;
                    salle[tmp].nb_conv_attendus = m_rest-> req_conv.taille_grp;
                }
            }

            // Si c'est pas un chef, cherche la table du chef
            else if (!refouler && places)
            {
                for (i = 0; i < nb_table; i++)
                {
                    if (strncmp(salle[i].liste_conv[0]
                            , m_rest -> req_conv.nom_chef, 10) == 0)
                    {
                        if (salle[i].nb_conv == salle[i].taille
                            || salle[i].nb_conv == salle[i].nb_conv_attendus)
                            tmp = -1;
                        else
                            tmp = i;
                        break;
                    }
                }

                if (tmp != -1)
                {
                    strncpy(salle[tmp].liste_conv[salle[tmp].nb_conv]
                            , m_rest -> req_conv.nom_convive, 11);
                    salle[tmp].nb_conv ++;
                }
            }

            // Si invalide envoi -1, sinon envoi indice table
            if ((tmp == -1 ) || refouler)
                m_rest -> reponse_serv = -1;
            else
            {
                m_rest -> reponse_serv = tmp; 
                if (salle[tmp].nb_conv == salle[tmp].nb_conv_attendus)
                {
                    demarrer_table = 1;
                    insert_cahier(cahier_rappel, salle[tmp]);
                    afficher_cahier(cahier_rappel);
                }
            }
            
            // Donne réponse au convive
            if (sem_post(&m_rest -> reponse_serveur) == -1)
                raler("sem_post reponse_serveur", 1);

            // Attend accusé de reception du convive
            if (sem_wait(&m_rest -> ack_convive) == -1)
                raler("sem_wait ack_convive", 1);
            
            if (demarrer_table)
            {
                nb_groupes_servis++;
                nb_convives_servis += salle[tmp].nb_conv;

                switch(fork())
                {
                    case -1 :
                        raler("fork", 1);
                        break;
                    case 0 :
                        lancer_chrono(duree_service, tmp, salle[tmp].nb_conv);
                        break;
                }
            }   
        }

        /**********************************************************************
         *                      TRAITE CONTROLE POLICE                        *
         *********************************************************************/
        
        if (sem_trywait(&m_rest -> police_presente) == -1)
        {
            if(errno != EAGAIN)
                raler("sem_trywait", 1);
        }
        else 
            // Fait le compte_rendu
            transmet_police(salle, cahier_rappel, nb_table);     

        /**********************************************************************
         *                      TRAITE COUVRE FEU                             *
         *********************************************************************/
        if (sem_trywait(&m_rest -> couvre_feu) == -1)
        {
            if(errno != EAGAIN)
                raler("sem_trywait", 1);
        }
        else
        {
            // Lancer les dernières tables et refouler
            refouler = 1;
            for (i = 0; i < nb_table; i++)
            {
                if (salle[i].nb_conv < salle[i].nb_conv_attendus)
                {
                    nb_groupes_servis++;
                    nb_convives_servis += salle[i].nb_conv;

                    insert_cahier(cahier_rappel, salle[i]);

                    switch(fork())
                    {
                        case -1 :
                            raler("fork", 1);
                            break;
                        case 0 :
                            lancer_chrono(duree_service, i, salle[i].nb_conv);
                            break;
                    } 
                }
            }

            // Fermeture
            ouverture_fermeture_restaurant(0);

            printf("%d convives servis dans %d groupes\n"
                    , nb_convives_servis, nb_groupes_servis);

            // Attendre tous les processus
            n_wait(nb_groupes_servis);

            // Unlink du restaurant
            destruction_restaurant(m_rest, nb_table);

            // Libération salle
            for (i = 0; i < nb_table; i++)
            {
                for (j = 0; j < salle[i].taille; j++)
                    free(salle[i].liste_conv[j]);
                free(salle[i].liste_conv);
            }
            free(salle);

            //Libération cahier de rappels
            for (i = 0; i < cahier_rappel -> nb_grp -2; i++)
            {   
                if (cahier_rappel -> head == NULL)
                    break;
                if (cahier_rappel -> head -> next == NULL)
                {
                    free(cahier_rappel -> head);
                    break;
                }
                elt_tmp = cahier_rappel -> head;
                cahier_rappel -> head = cahier_rappel -> head -> next;
                free(elt_tmp);
            }
            free(cahier_rappel);

            return EXIT_SUCCESS;
        }
    
    }// Fin boucle principal

    return EXIT_SUCCESS; // Normalement pas atteint
}

#endif