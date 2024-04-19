#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>
#include <thread>
#include <mutex>

static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 200;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075;
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025;
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;

// Type definitions
enum entity_type_t
{
    empty,
    plant,
    herbivore,
    carnivore
};

struct pos_t
{
    uint32_t i;
    uint32_t j;
    // Sobrecarga do operador de igualdade
    bool operator==(const pos_t& other) const {
        return i == other.i && j == other.j;
    }
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
    bool already_atualized;
};
std::mutex mtx;
// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {empty, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;

bool random_action(float probability) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < probability;
}

std::mutex cv_mutex;
std::condition_variable cv;

void simul_plant(int i, int j) {

    std::unique_lock<std::mutex> lock(cv_mutex);
    printf("parou no wait planta\n");
    cv.wait(lock);

    std::vector<pos_t> growth_positions_available;
    printf("saiu no wait planta\n");

    if(i+1 < NUM_ROWS) {
        if(entity_grid[i+1][j].type == empty) {
            pos_t position_available;
            position_available.i = i+1;
            position_available.j = j;
            growth_positions_available.push_back(position_available);
        }
    }

    if(i > 0) {
        if(entity_grid[i-1][j].type == empty) {
            pos_t position_available;
            position_available.i = i-1;
            position_available.j = j;
            growth_positions_available.push_back(position_available);
        }
    }

    if(j+1 < NUM_ROWS) {
        if(entity_grid[i][j+1].type == empty){
            pos_t position_available;
            position_available.i = i;
            position_available.j = j+1;
            growth_positions_available.push_back(position_available);
        }
    }

    if(j > 0) {
        if(entity_grid[i][j-1].type == empty) {
            pos_t position_available;
            position_available.i = i;
            position_available.j = j-1;
            growth_positions_available.push_back(position_available);
        }
    }

    if (!growth_positions_available.empty()) {
        bool try_to_reproduct = random_action(PLANT_REPRODUCTION_PROBABILITY);
        if(try_to_reproduct == true) {
            std::random_device rd;  
            std::mt19937 gen(rd());  
            std::uniform_int_distribution<> dis(0, growth_positions_available.size() - 1);
            pos_t sorted_position = growth_positions_available[dis(gen)];
        
            entity_grid[sorted_position.i][sorted_position.j].type = plant;
            entity_grid[sorted_position.i][sorted_position.j].energy = 0;
            entity_grid[sorted_position.i][sorted_position.j].age = 0;
            entity_grid[sorted_position.i][sorted_position.j].already_atualized = true;
        }
    }

    entity_grid[i][j].age += 1;  // aumenta a idade da planta em 1

    if (entity_grid[i][j].age == PLANT_MAXIMUM_AGE) {  // verifica se a planta atingiu a idade maxima e se sim a planta morre
        entity_grid[i][j].type = empty;
        entity_grid[i][j].energy = 0;
        entity_grid[i][j].age = 0;
    }

    // Libera os mutexes das posições adjacentes
    printf("terminou planta planta\n");


}

void simul_herbivore(int i, int j) {
    std::unique_lock<std::mutex> lock(cv_mutex);
    printf("parou no wait herbivoro\n");
    cv.wait(lock);
    printf("saiu no wait herbivoro\n");
    std::vector<pos_t> neighboring_empty_positions; // vetor das posicoes vazias adjacentes
    std::vector<pos_t> neighboring_plants_positions; // vetor das posicoes com planta adjacentes

    if(i+1 < NUM_ROWS) {
        if(entity_grid[i+1][j].type == empty) {
            pos_t position_available;
            position_available.i = i+1;
            position_available.j = j;
            neighboring_empty_positions.push_back(position_available);
        }
        if(entity_grid[i+1][j].type == plant) {
            pos_t position_available;
            position_available.i = i+1;
            position_available.j = j;
            neighboring_plants_positions.push_back(position_available);
        }
    }

    if(i > 0) {
        if(entity_grid[i-1][j].type == empty) {
            pos_t position_available;
            position_available.i = i-1;
            position_available.j = j;
            neighboring_empty_positions.push_back(position_available);
        }
        if(entity_grid[i-1][j].type == plant) {
            pos_t position_available;
            position_available.i = i-1;
            position_available.j = j;
            neighboring_plants_positions.push_back(position_available);
        }
    }

    if(j+1 < NUM_ROWS) {
        if(entity_grid[i][j+1].type == empty){
            pos_t position_available;
            position_available.i = i;
            position_available.j = j+1;
            neighboring_empty_positions.push_back(position_available);
        }
        if(entity_grid[i][j+1].type == plant){
            pos_t position_available;
            position_available.i = i;
            position_available.j = j+1;
            neighboring_plants_positions.push_back(position_available);
        }
    }

    if(j > 0) {
        if(entity_grid[i][j-1].type == empty) {
            pos_t position_available;
            position_available.i = i;
            position_available.j = j-1;
            neighboring_empty_positions.push_back(position_available);
        }if(entity_grid[i][j-1].type == plant) {
            pos_t position_available;
            position_available.i = i;
            position_available.j = j-1;
            neighboring_plants_positions.push_back(position_available);
        }
    }

    // tentativa de comer uma planta
    if (!neighboring_plants_positions.empty()) {
        bool try_to_eat = random_action(HERBIVORE_EAT_PROBABILITY);
        if(try_to_eat == true) {
            std::random_device rd;  
            std::mt19937 gen(rd());  
            std::uniform_int_distribution<> dis(0, neighboring_plants_positions.size() - 1);
            pos_t eat_position = neighboring_plants_positions[dis(gen)];
        
            entity_grid[eat_position.i][eat_position.j].type = empty;
            entity_grid[eat_position.i][eat_position.j].energy = 0;
            entity_grid[eat_position.i][eat_position.j].age = 0;
            entity_grid[eat_position.i][eat_position.j].already_atualized = true;

            entity_grid[i][j].energy = entity_grid[i][j].energy + 30;
            neighboring_empty_positions.push_back(eat_position);
        }
    }

    // tentativa de se reproduzir
    if (!neighboring_empty_positions.empty()) {
        if(entity_grid[i][j].energy > THRESHOLD_ENERGY_FOR_REPRODUCTION) {
            bool try_to_reproduce = random_action(HERBIVORE_REPRODUCTION_PROBABILITY);
            if(try_to_reproduce == true) {
                std::random_device rd;  
                std::mt19937 gen(rd());  
                std::uniform_int_distribution<> dis(0, neighboring_empty_positions.size() - 1);
                pos_t child_position = neighboring_empty_positions[dis(gen)];
            
                entity_grid[child_position.i][child_position.j].type = herbivore;
                entity_grid[child_position.i][child_position.j].energy = 100;
                entity_grid[child_position.i][child_position.j].age = 0;
                entity_grid[child_position.i][child_position.j].already_atualized = true;

                entity_grid[i][j].energy = entity_grid[i][j].energy - 10;
                for (auto it = neighboring_empty_positions.begin(); it != neighboring_empty_positions.end(); ++it) {
                        if (*it == child_position) {
                        neighboring_empty_positions.erase(it);
                        break;  
                    }
                }
            }
        }
    }

    bool age_increased = false;

    // tentativa de se movimentar
    if (!neighboring_empty_positions.empty()) {
        bool try_to_move = random_action(HERBIVORE_MOVE_PROBABILITY);
        if(try_to_move == true) {
            std::random_device rd;  
            std::mt19937 gen(rd());  
            std::uniform_int_distribution<> dis(0, neighboring_empty_positions.size() - 1);
            pos_t move_position = neighboring_empty_positions[dis(gen)];
        
            entity_grid[move_position.i][move_position.j].type = herbivore;
            entity_grid[move_position.i][move_position.j].energy = entity_grid[i][j].energy - 5;
            entity_grid[move_position.i][move_position.j].age = entity_grid[i][j].age + 1;
            entity_grid[move_position.i][move_position.j].already_atualized = true;
            age_increased = true;

            entity_grid[i][j].type = empty;
            entity_grid[i][j].energy = 0;
            entity_grid[i][j].age = 0;
            
            if (entity_grid[move_position.i][move_position.j].age == HERBIVORE_MAXIMUM_AGE || entity_grid[move_position.i][move_position.j].energy == 0) { 
                entity_grid[move_position.i][move_position.j].type = empty;
                entity_grid[move_position.i][move_position.j].energy = 0;
                entity_grid[move_position.i][move_position.j].age = 0;
            }
        }
    }
    if(age_increased == false) {
        entity_grid[i][j].age += 1;
    }

    if (entity_grid[i][j].age == HERBIVORE_MAXIMUM_AGE || entity_grid[i][j].energy == 0) { 
        entity_grid[i][j].type = empty;
        entity_grid[i][j].energy = 0;
        entity_grid[i][j].age = 0;
    }
    printf("terminou herbivoro\n");
}

void simul_carnivore(int i, int j) {
    std::unique_lock<std::mutex> lock(cv_mutex);
    printf("parou wait carnivoro\n");
    cv.wait(lock);
    printf("saiu do wait carnivoro\n");

    std::vector<pos_t> neighboring_empty_positions; // vetor das posicoes vazias adjacentes
    std::vector<pos_t> neighboring_herbivore_positions; // vetor das posicoes com herbivoros adjacentes

    if(i+1 < NUM_ROWS) {
        if(entity_grid[i+1][j].type == empty) {
            pos_t position_available;
            position_available.i = i+1;
            position_available.j = j;
            neighboring_empty_positions.push_back(position_available);
        }
        if(entity_grid[i+1][j].type == herbivore) {
            pos_t position_available;
            position_available.i = i+1;
            position_available.j = j;
            neighboring_herbivore_positions.push_back(position_available);
        }
    }

    if(i > 0) {
        if(entity_grid[i-1][j].type == empty) {
            pos_t position_available;
            position_available.i = i-1;
            position_available.j = j;
            neighboring_empty_positions.push_back(position_available);
        }
        if(entity_grid[i-1][j].type == herbivore) {
            pos_t position_available;
            position_available.i = i-1;
            position_available.j = j;
            neighboring_herbivore_positions.push_back(position_available);
        }
    }

    if(j+1 < NUM_ROWS) {
        if(entity_grid[i][j+1].type == empty){
            pos_t position_available;
            position_available.i = i;
            position_available.j = j+1;
            neighboring_empty_positions.push_back(position_available);
        }
        if(entity_grid[i][j+1].type == herbivore){
            pos_t position_available;
            position_available.i = i;
            position_available.j = j+1;
            neighboring_herbivore_positions.push_back(position_available);
        }
    }

    if(j > 0) {
        if(entity_grid[i][j-1].type == empty) {
            pos_t position_available;
            position_available.i = i;
            position_available.j = j-1;
            neighboring_empty_positions.push_back(position_available);
        }if(entity_grid[i][j-1].type == herbivore) {
            pos_t position_available;
            position_available.i = i;
            position_available.j = j-1;
            neighboring_herbivore_positions.push_back(position_available);
        }
    }

    // tentativa de comer um herbivoro
    if (!neighboring_herbivore_positions.empty()) {
        bool try_to_eat = random_action(CARNIVORE_EAT_PROBABILITY);
        if(try_to_eat == true) {
            std::random_device rd;  
            std::mt19937 gen(rd());  
            std::uniform_int_distribution<> dis(0, neighboring_herbivore_positions.size() - 1);
            pos_t eat_position = neighboring_herbivore_positions[dis(gen)];
        
            entity_grid[eat_position.i][eat_position.j].type = empty;
            entity_grid[eat_position.i][eat_position.j].energy = 0;
            entity_grid[eat_position.i][eat_position.j].age = 0;
            entity_grid[eat_position.i][eat_position.j].already_atualized = true;

            entity_grid[i][j].energy = entity_grid[i][j].energy + 20;
            neighboring_empty_positions.push_back(eat_position);
        }
    }

    // tentativa de se reproduzir
    if (!neighboring_empty_positions.empty()) {
        if(entity_grid[i][j].energy > THRESHOLD_ENERGY_FOR_REPRODUCTION) {
            bool try_to_reproduce = random_action(CARNIVORE_REPRODUCTION_PROBABILITY);
            if(try_to_reproduce == true) {
                std::random_device rd;  
                std::mt19937 gen(rd());  
                std::uniform_int_distribution<> dis(0, neighboring_empty_positions.size() - 1);
                pos_t child_position = neighboring_empty_positions[dis(gen)];
            
                entity_grid[child_position.i][child_position.j].type = carnivore;
                entity_grid[child_position.i][child_position.j].energy = 100;
                entity_grid[child_position.i][child_position.j].age = 0;
                entity_grid[child_position.i][child_position.j].already_atualized = true;

                entity_grid[i][j].energy = entity_grid[i][j].energy - 10;
                for (auto it = neighboring_empty_positions.begin(); it != neighboring_empty_positions.end(); ++it) {
                        if (*it == child_position) {
                        neighboring_empty_positions.erase(it);
                        break;  
                    }
                }
            }
        }
    }

    bool age_increased = false;

    // tentativa de se movimentar
    if (!neighboring_empty_positions.empty()) {
        bool try_to_move = random_action(CARNIVORE_MOVE_PROBABILITY);
        if(try_to_move == true) {
            std::random_device rd;  
            std::mt19937 gen(rd());  
            std::uniform_int_distribution<> dis(0, neighboring_empty_positions.size() - 1);
            pos_t move_position = neighboring_empty_positions[dis(gen)];
        
            entity_grid[move_position.i][move_position.j].type = carnivore;
            entity_grid[move_position.i][move_position.j].energy = entity_grid[i][j].energy - 5;
            entity_grid[move_position.i][move_position.j].age = entity_grid[i][j].age + 1;
            entity_grid[move_position.i][move_position.j].already_atualized = true;
            age_increased = true;

            entity_grid[i][j].type = empty;
            entity_grid[i][j].energy = 0;
            entity_grid[i][j].age = 0;
            
            if (entity_grid[move_position.i][move_position.j].age == CARNIVORE_MAXIMUM_AGE || entity_grid[move_position.i][move_position.j].energy == 0) { 
                entity_grid[move_position.i][move_position.j].type = empty;
                entity_grid[move_position.i][move_position.j].energy = 0;
                entity_grid[move_position.i][move_position.j].age = 0;
            }
        }
    }
    if(age_increased == false) {
        entity_grid[i][j].age += 1;
    }

    if (entity_grid[i][j].age == CARNIVORE_MAXIMUM_AGE || entity_grid[i][j].energy == 0) { 
        entity_grid[i][j].type = empty;
        entity_grid[i][j].energy = 0;
        entity_grid[i][j].age = 0;
    }

    // Libera os mutexes das posições adjacentes
    printf("terminou carnivoro\n");
}

int main()
{
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end(); });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
                                { 
        // Parse the JSON request body
        nlohmann::json request_body = nlohmann::json::parse(req.body);

       // Validate the request body 
        uint32_t total_entinties = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
        if (total_entinties > NUM_ROWS * NUM_ROWS) {
        res.code = 400;
        res.body = "Too many entities";
        res.end();
        return;
        }

        // Clear the entity grid
        entity_grid.clear();
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0, false}));
        
        // Create the entities
        for(int i = 0; i < (uint32_t)request_body["plants"]; i++) {
            static std::random_device rd; // Inicializa a random_device para obter sementes aleatórias
            static std::mt19937 gen(rd()); // Usa a random_device para inicializar um gerador de números pseudoaleatórios
            std::uniform_int_distribution<> dis(0, 14); //Gera um número aleatório no intervalo definido
            int rand_row = dis(gen);
            int rand_col = dis(gen);

            while(entity_grid[rand_row][rand_col].type != empty){
                rand_row = dis(gen);
                rand_col = dis(gen);
            }
            
            entity_grid[rand_row][rand_col].type = plant;
            entity_grid[rand_row][rand_col].age = 0; 
        }

        for(int i = 0; i < (uint32_t)request_body["herbivores"]; i++) {
            static std::random_device rd; 
            static std::mt19937 gen(rd()); 
            std::uniform_int_distribution<> dis(0, 14); 
            int rand_row = dis(gen);
            int rand_col = dis(gen);

            while(entity_grid[rand_row][rand_col].type != empty){
                rand_row = dis(gen);
                rand_col = dis(gen);
            }
            
            entity_grid[rand_row][rand_col].type = herbivore;
            entity_grid[rand_row][rand_col].age = 0; 
            entity_grid[rand_row][rand_col].energy = 100; 
        }

        for(int i = 0; i < (uint32_t)request_body["carnivores"]; i++) {
            static std::random_device rd; 
            static std::mt19937 gen(rd()); 
            std::uniform_int_distribution<> dis(0, 14); 
            int rand_row = dis(gen);
            int rand_col = dis(gen);

            while(entity_grid[rand_row][rand_col].type != empty){
                rand_row = dis(gen);
                rand_col = dis(gen);
            }
            
            entity_grid[rand_row][rand_col].type = carnivore;
            entity_grid[rand_row][rand_col].age = 0; 
            entity_grid[rand_row][rand_col].energy = 100; 
        }

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        res.body = json_grid.dump();
        res.end(); });

    // Endpoint to process HTTP GET requests for the next simulation iteration
    CROW_ROUTE(app, "/next-iteration")
        .methods("GET"_method)([]()
                               {
        // Simulate the next iteration
        // Iterate over the entity grid and simulate the behaviour of each entity
        for (int i=0; i<15; i++) {
            for (int j=0; j<15; j++) {
                entity_grid[i][j].already_atualized = false;
            }
        }

        std::vector<std::thread> threads;
        for (int i = 0; i < NUM_ROWS; i++) {
            for (int j = 0; j < NUM_ROWS; j++) {
                if (!entity_grid[i][j].already_atualized) {
                    if (entity_grid[i][j].type == plant) {
                        threads.emplace_back(simul_plant, i, j);
                        printf("criou a thread da planta\n");
                    }
                    else if (entity_grid[i][j].type == herbivore) {
                        threads.emplace_back(simul_herbivore, i, j);
                        printf("criou a thread do herbivoro\n");
                    }
                    else if (entity_grid[i][j].type == carnivore) {
                        threads.emplace_back(simul_carnivore, i, j);
                        printf("criou a thread do carnivoro\n");
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int i=0;
        printf("acordou todas\n");        
        cv.notify_all();
        
        for (auto& thread : threads) {
        printf("join thread %d\n", i);
        thread.join();
        i++;
        }      
        







        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}