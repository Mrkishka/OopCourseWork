#include <iostream>
#include <vector>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <random>
#include <thread>
#include <chrono>

constexpr int FIELD_SIZE = 10;
constexpr int MIN_WOLVES = 5;
constexpr int MIN_RABBITS = 5;

class Creature
{
protected:
    int x, y;

public:
    Creature(int x, int y) : x(x), y(y) {}
    virtual ~Creature() = default;
    virtual void moveAndAct(std::vector<std::vector<std::shared_ptr<Creature>>> &field) = 0;
};

class Carrot : public Creature
{
private:
    int lifeSpan;

public:
    Carrot(int x, int y) : Creature(x, y), lifeSpan(5) {}

    void moveAndAct(std::vector<std::vector<std::shared_ptr<Creature>>> &field) override
    {
        if (--lifeSpan <= 0)
        {
            field[x][y].reset();
        }
    }
};

class Rabbit : public Creature
{
private:
    float hunger;
    static std::mt19937 rng;

public:
    Rabbit(int x, int y) : Creature(x, y), hunger(1.0) {}

    void moveAndAct(std::vector<std::vector<std::shared_ptr<Creature>>> &field) override
    {
        if (hunger <= 0)
        {
            field[x][y].reset();
            return;
        }

        std::uniform_int_distribution<int> dist(-1, 1);
        int dx = dist(rng);
        int dy = dist(rng);
        int nx = (x + dx + FIELD_SIZE) % FIELD_SIZE;
        int ny = (y + dy + FIELD_SIZE) % FIELD_SIZE;

        if (!field[nx][ny] || dynamic_cast<Carrot *>(field[nx][ny].get()))
        {
            if (dynamic_cast<Carrot *>(field[nx][ny].get()))
            {
                hunger += 0.2;
            }
            field[nx][ny] = field[x][y];
            field[x][y].reset();
            x = nx;
            y = ny;
        }

        hunger -= 0.2;
    }
};

std::mt19937 Rabbit::rng(std::random_device{}());

class Wolf : public Creature
{
private:
    float hunger;
    int eatenHares;
    bool hasReproduced;
    static std::mt19937 rng;

public:
    Wolf(int x, int y) : Creature(x, y), hunger(2.0), eatenHares(0), hasReproduced(false) {}

    void moveAndAct(std::vector<std::vector<std::shared_ptr<Creature>>> &field) override
    {
        if (hunger <= 0)
        {
            field[x][y].reset();
            return;
        }

        int moveRange = (hunger < 0.5) ? 2 : 1;
        std::uniform_int_distribution<int> dist(-moveRange, moveRange);

        for (int attempt = 0; attempt < 5; ++attempt)
        {
            int dx = dist(rng);
            int dy = dist(rng);

            if (dx == 0 && dy == 0)
                continue;

            int nx = (x + dx + FIELD_SIZE) % FIELD_SIZE;
            int ny = (y + dy + FIELD_SIZE) % FIELD_SIZE;

            if (!field[nx][ny] || dynamic_cast<Rabbit *>(field[nx][ny].get()))
            {
                if (dynamic_cast<Rabbit *>(field[nx][ny].get()))
                {
                    hunger += 0.4;
                    ++eatenHares;
                    if (eatenHares > 2 && !hasReproduced)
                    {
                        hasReproduced = true;
                        field[x][y] = std::make_shared<Wolf>(x, y);
                    }
                }

                field[nx][ny] = field[x][y];
                field[x][y].reset();
                x = nx;
                y = ny;
                break;
            }
        }

        hunger -= 0.2;
    }
};

std::mt19937 Wolf::rng(std::random_device{}());

void addNewCarrots(std::vector<std::vector<std::shared_ptr<Creature>>> &field)
{
    for (int i = 0; i < 5; ++i)
    {
        int x = rand() % FIELD_SIZE;
        int y = rand() % FIELD_SIZE;
        if (!field[x][y])
        {
            field[x][y] = std::make_shared<Carrot>(x, y);
        }
    }
}

void ensureRabbitBalance(std::vector<std::vector<std::shared_ptr<Creature>>> &field)
{
    int rabbitCount = 0;

    for (int i = 0; i < FIELD_SIZE; ++i)
    {
        for (int j = 0; j < FIELD_SIZE; ++j)
        {
            if (dynamic_cast<Rabbit *>(field[i][j].get()))
            {
                ++rabbitCount;
            }
        }
    }

    while (rabbitCount < MIN_RABBITS)
    {
        int x = rand() % FIELD_SIZE;
        int y = rand() % FIELD_SIZE;
        if (!field[x][y])
        {
            field[x][y] = std::make_shared<Rabbit>(x, y);
            ++rabbitCount;
        }
    }
}

void ensureMinimumWolves(std::vector<std::vector<std::shared_ptr<Creature>>> &field)
{
    int wolfCount = 0;

    for (int i = 0; i < FIELD_SIZE; ++i)
    {
        for (int j = 0; j < FIELD_SIZE; ++j)
        {
            if (dynamic_cast<Wolf *>(field[i][j].get()))
            {
                ++wolfCount;
            }
        }
    }

    while (wolfCount < MIN_WOLVES)
    {
        for (int i = 0; i < FIELD_SIZE; ++i)
        {
            if (!field[i][0])
            {
                field[i][0] = std::make_shared<Wolf>(i, 0);
                ++wolfCount;
                break;
            }
            if (!field[i][FIELD_SIZE - 1])
            {
                field[i][FIELD_SIZE - 1] = std::make_shared<Wolf>(i, FIELD_SIZE - 1);
                ++wolfCount;
                break;
            }
        }
    }
}

void printField(const std::vector<std::vector<std::shared_ptr<Creature>>> &field)
{
    for (int i = 0; i < FIELD_SIZE; ++i)
    {
        for (int j = 0; j < FIELD_SIZE; ++j)
        {
            if (dynamic_cast<Carrot *>(field[i][j].get()))
                std::cout << "C ";
            else if (dynamic_cast<Rabbit *>(field[i][j].get()))
                std::cout << "R ";
            else if (dynamic_cast<Wolf *>(field[i][j].get()))
                std::cout << "W ";
            else
                std::cout << ". ";
        }
        std::cout << "\n";
    }
    std::cout << "=========================\n";
}

int main()
{
    std::srand(std::time(0));
    std::vector<std::vector<std::shared_ptr<Creature>>> field(FIELD_SIZE, std::vector<std::shared_ptr<Creature>>(FIELD_SIZE));

    for (int i = 0; i < 15; ++i)
    {
        int x = rand() % FIELD_SIZE;
        int y = rand() % FIELD_SIZE;
        field[x][y] = std::make_shared<Carrot>(x, y);
    }

    for (int i = 0; i < 10; ++i)
    {
        int x = rand() % FIELD_SIZE;
        int y = rand() % FIELD_SIZE;
        field[x][y] = std::make_shared<Rabbit>(x, y);
    }

    for (int i = 0; i < 5; ++i)
    {
        int x = rand() % FIELD_SIZE;
        int y = rand() % FIELD_SIZE;
        field[x][y] = std::make_shared<Wolf>(x, y);
    }

    while (true)
    {
        for (int i = 0; i < FIELD_SIZE; ++i)
        {
            for (int j = 0; j < FIELD_SIZE; ++j)
            {
                if (field[i][j])
                    field[i][j]->moveAndAct(field);
            }
        }

        addNewCarrots(field);
        ensureRabbitBalance(field);
        ensureMinimumWolves(field);

        printField(field);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    return 0;
}
