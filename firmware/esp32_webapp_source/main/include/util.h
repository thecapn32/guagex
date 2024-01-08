#ifdef __cplusplus
extern "C" {
#endif

#define NAME_MAX_LEN 100
#define POS_MAX_LEN 5
#define COEFF_CNT 4

enum unit_t
{
    PSI,
    BAR,
    CELSIUS,
    VOLTAGE,
    AFR,
    LAMBDA,
    FAHRENHEIT
};

struct channel_info_t
{
    int min;
    int max;
    enum unit_t unit;
    char pos[POS_MAX_LEN];
    char name[NAME_MAX_LEN];
    double coeff[COEFF_CNT];
};

char *unit_id_str(int id);
int unit_str_id(char *u);

void print_obj(struct channel_info_t *t);

#ifdef __cplusplus
}
#endif