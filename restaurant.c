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

    time.tv_sec += duree_service/1000;
    time.tv_nsec += duree_service * 1000000;
    time.tv_nsec %= 1000000000;


    if (sem_timedwait(&faux_sem, &time) == -1)
        if (errno != ETIMEDOUT)
            raler("timedwait", 1);

    for(i = 0; i < nb_conv; i++)
    {
        if (sem_post(&restaurant -> fin_repas[indice]) == -1)
            raler("sem_post fin_repas", 1);
        printf("LE CONVIVE %d S'EN VA\n", i);
    }

    if (sem_destroy(&faux_sem) == -1)
        raler("sem_destroy faux_sem", 1);

    restaurant_unmap(restaurant);

    exit(EXIT_SUCCESS);
}

void affiche_salle(struct table *salle, int nb_table)
{
    int i, j;
    for (i = 0; i < nb_table; i++)
    {
        printf("\nTable %d\ntaille : %d, nb_conv : %d\n", i
                , salle[i].taille, salle[i].nb_conv);
        for(j = 0; j < salle[i].taille; j++)
        {
            printf("convive %d : %s |", j, salle[i].liste_conv[j]);
        }
        printf("\n");
    }
    printf("---------------------------------------------------------\n");
}

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

void transmet_police(struct table *salle, struct compte_rendu *cr, int nb_tab)
{
    int i, j, fd, nb_elem = nb_tab + cr -> nb_grp;
    ssize_t octets_cr = SIZE_COMPTE_RENDU(nb_elem);
    struct element *tmp = cr -> head;
    struct compte_rendu_shm *cr_shm;
    struct restaurant *restaurant = restaurant_map();

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

    printf("Ecriture du compte rendu \n");
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
                        , salle[i].liste_conv[j], 10);
            cr_shm -> liste_elements[i].noms[j][10] = '\0'; //securité
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
                        , tmp -> noms[j], 10);
            cr_shm -> liste_elements[nb_tab + i].noms[j][10] = '\0';//securité            
        }
        i++;
        tmp = tmp -> next;
    }

    printf("compte rendu pret\n");
    // Signale compte_rendu pret
    if (sem_post(&restaurant -> compte_rendu_pret) == -1)
        raler("sem_post compte_rendu_pret", 1);

    if (sem_wait(&cr_shm -> ack_police) == -1)
        raler("sem_wait ack_police", 1);

    printf("police a bien reçu, je quitte\n");

    // Désallocations
    restaurant_unmap(restaurant);

    if (sem_destroy(&cr_shm -> ack_police) == -1)
        raler("sem_destroy ack_police", 1);

    if (munmap(cr_shm, octets_cr) == -1)
        raler("munmap", 1);

    if  (shm_unlink(NOM_COMPTE_RENDU) == -1)
        raler("shm_unlink", 1);    
}

void ajout_groupe_servi(struct compte_rendu *cr, struct table t)
{
    int i;
    struct element *elt = malloc(sizeof(struct element));
    elt -> nb_conv = t.nb_conv;
    elt -> next = NULL;
    printf("DEBUT FOR on recopie %s\n", t.liste_conv[0]);
    for (i = 0; i < t.nb_conv; i++)
    {
        strncpy(elt -> noms[i], t.liste_conv[i], 10);
        elt -> noms[i][10] = '\0'; //sécurité
        printf("%s a bien été recopié\n", elt -> noms[i]);
    }

    cr -> nb_grp++;
    cr -> tail -> next = elt;//printf("2\n");
    cr -> tail = elt;//printf("3\n");
}

int main(int argc, char *argv[])
{
    int nb_table = 0, test_convive = 0, test_couvrefeu = 0, i, j;
    int tmp, demarrer_table, nb_groupes_servis = 0, nb_convives_servis = 0;
    int taille_tmp, refouler = 0;
    long int duree_service = 0, *table_sizes;
    char *endptr, *str;
    struct restaurant *m_rest;
    struct table *salle;
    struct compte_rendu *cahier_rappel;

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
    cahier_rappel = malloc(sizeof(struct compte_rendu));
    cahier_rappel -> nb_grp = 0;
    cahier_rappel -> head = malloc(sizeof(struct element));
    cahier_rappel -> tail = malloc(sizeof(struct element));

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

    affiche_salle(salle, nb_table);

    // Création restaurant puis ouverture
    m_rest = creation_restaurant(nb_table);
    ouverture_fermeture_restaurant(1);




    /**************************************************************************
     *                          DEBUT TRAVAILLE                               *
     *************************************************************************/
    while (!test_couvrefeu)
    {
        /**********************************************************************
         *                 NETTOYAGE DES TABLES VIDES                         *
         *********************************************************************/
        for (i = 0; i < nb_table; i++)
        {
            if (sem_getvalue(&m_rest -> fin_repas[i], &test_convive) == -1)
                raler("sem_getvalue", 1);
            if (test_convive == 0 && salle[i].nb_conv != 0)
            {
                salle[i].nb_conv = 0;
                salle[i].nb_conv_attendus = 0;
            }
        }

        /**********************************************************************
         *                      CHECK CONVIVE ATTEND                          *
         *********************************************************************/
        if (sem_getvalue(&m_rest -> serveur_dispo, &test_convive) == -1)
            raler("sem_getvalue", 1);

        if (test_convive < 1)
        {
            printf("COUVRE FEU\n");

            if (sem_post(&m_rest -> serveur_dispo) == -1)
                raler("sem_post serveur_dispo", 1);
        
            if (sem_wait(&m_rest -> requete_ecrite) == -1)
                raler("sem_wait requete_ecrite", 1);

            printf("nom convive : %s\nnom chef : %s\ntaille groupe : %d\n"
                    , m_rest -> req_conv.nom_convive
                    , m_rest -> req_conv.nom_chef
                    , m_rest -> req_conv.taille_grp);
            
            tmp = -1;
            taille_tmp = 7;
            demarrer_table = 0;
            // si c'est un chef cherche la table qui va bien
            if ((m_rest -> req_conv.taille_grp != -1) && !refouler)
            {
                printf("C'EST LE CHEF\n");
                for (i = 0; i < nb_table; i++)
                {
                    if(salle[i].nb_conv == 0 // dispo && taille && + optimisée
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
                            , m_rest -> req_conv.nom_convive, 10);
                    salle[tmp].liste_conv[0][10] = '\0'; // sécurité
                    salle[tmp].nb_conv ++;
                    salle[tmp].nb_conv_attendus = m_rest-> req_conv.taille_grp;
                    //ECRIRE AUSSI DANS LE CAHIER DE RAPPEL
                }
            }

            // si c'est pas un chef, cherche la table du chef
            else if (!refouler)
            {
                printf("C'EST PAS LE CHEF\n");
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
                            , m_rest -> req_conv.nom_convive, 10);
                    salle[tmp].liste_conv[salle[tmp].nb_conv][10] = '\0';
                    salle[tmp].nb_conv ++;
                    //ECRIRE AUSSI DANS LE CAHIER DE RAPPEL
                }
            }

            // pas de table -> refouler
            if ((tmp == -1 )|| refouler)
                m_rest -> reponse_serv = -1;
            // sinon -> donner indice de la table
            else
            {
                m_rest -> reponse_serv = tmp; 
                if (salle[tmp].nb_conv == salle[tmp].nb_conv_attendus)
                    demarrer_table = 1;
                printf("AJOUT AU CAHIER\n");
                ajout_groupe_servi(cahier_rappel, salle[tmp]);
            }
            

            // donne réponse au convive
            if (sem_post(&m_rest -> reponse_serveur) == -1)
                raler("sem_post reponse_serveur", 1);


            // attend accusé de reception du convive
            if (sem_wait(&m_rest -> ack_convive) == -1)
                raler("sem_wait ack_convive", 1);
            
            // NETTOYER REQUETE


            if (demarrer_table)
            {
                nb_groupes_servis++;
                nb_convives_servis += salle[tmp].nb_conv;
                printf("JE DEMARRE TABLE\n");
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
            affiche_salle(salle, nb_table);
        }

        /**********************************************************************
         *                      CHECK CONTROLE POLICE                         *
         *********************************************************************/
        if (sem_trywait(&m_rest -> police_presente) == -1)
        {
            if(errno != EAGAIN)
                raler("sem_trywait", 1);
        }
        else
        {
            printf("LA POLICE\n");

            // Fait le compte_rendu
            transmet_police(salle, cahier_rappel, nb_table);

            printf("le controle s'est bien passé je reprends l'attente...\n");


        }

        /**********************************************************************
         *                      CHECK COUVRE FEU                              *
         *********************************************************************/
        if (sem_trywait(&m_rest -> couvre_feu) == -1)
        {
            if(errno != EAGAIN)
                raler("sem_trywait", 1);
        }
        else
        {
            printf("COUVRE FEU\n");
            // LANCER TABLES ET REFOULE
            refouler = 1;
            for (i = 0; i < nb_table; i++)
            {
                if (salle[i].nb_conv < salle[i].nb_conv_attendus)
                {
                    nb_groupes_servis++;
                    nb_convives_servis += salle[i].nb_conv;

                    ajout_groupe_servi(cahier_rappel, salle[i]);

                    printf("JE DEMARRE TABLE CAR COUVRE FEU APPROCHE\n");
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


            //ATTENDRE FIN TABLES
            // for (int i = 0; i < nb_table; i++)
            // {
            //     while()
            // }

            ouverture_fermeture_restaurant(0); //fermeture

            printf("%d convives servis dans %d groupes\n"
                    , nb_convives_servis, nb_groupes_servis);
            //print_debug(1, "destruction restaurant");



            n_wait(nb_groupes_servis);

            // FREE LES STRUCTURES MALLOC           
            destruction_restaurant(m_rest, nb_table);
            return EXIT_SUCCESS;
        }
    
    }//fin while


    //print_debug(1, "resturant détruit");

    return EXIT_SUCCESS;
}

#endif