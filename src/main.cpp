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
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
    std::mutex *mut;
};

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

void simul_plant(int i, int j) {

    std::vector<pos_t> growth_positions_available;

    if(i+1 < 15 && entity_grid[i+1][j].type == empty) {
        pos_t position_available;
        position_available.i = i+1;
        position_available.j = j;
        growth_positions_available.push_back(position_available);
    }

    if(i-1 >= 0 && entity_grid[i-1][j].type == empty) {
        pos_t position_available;
        position_available.i = i-1;
        position_available.j = j;
        growth_positions_available.push_back(position_available);
    }

    if(j+1 < 15 && entity_grid[i][j+1].type == empty) {
        pos_t position_available;
        position_available.i = i;
        position_available.j = j+1;
        growth_positions_available.push_back(position_available);
    }

    if(j-1 >= 0 && entity_grid[i][j+1].type == empty) {
        pos_t position_available;
        position_available.i = i;
        position_available.j = j-1;
        growth_positions_available.push_back(position_available);
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
        }
    }

    entity_grid[i][j].age += 1;  //increase current plant age

    if (entity_grid[i][j].age == PLANT_MAXIMUM_AGE) {  // verify if the plant achieved the death age
        entity_grid[i][j].type = empty;
        entity_grid[i][j].energy = 0;
        entity_grid[i][j].age = 0;
    }
}

void simul_herbivore(int i, int j) {
    
}

void simul_carnivore(int i, int j) {
    
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
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0}));
        
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
        for (int i=0; i<NUM_ROWS; i++) {
            for (int j=0; j<NUM_ROWS; j++) {
                if (entity_grid[i][j].type == plant) {
                    std::thread t_plant(simul_plant, i, j);
                    t_plant.detach();
                }
                else if (entity_grid[i][j].type == herbivore) {          
                    std::thread t_herbivore(simul_herbivore, i, j);
                    t_herbivore.detach();
                }
                else if (entity_grid[i][j].type == carnivore) {
                    std::thread t_carnivore(simul_carnivore, i, j);
                    t_carnivore.detach();
                }                    
            }
        }
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}