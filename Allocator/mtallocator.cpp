#include <cstdio>
#include <thread>
#include <mutex>
#include <vector>
#include <cassert>


const size_t HEAPS_COUNT = std::thread::hardware_concurrency() * 2;
const size_t SUPERBLOCK_SIZE = 8192; //удвоенный размер страницы памяти - почему бы и нет
const size_t GRAND_HEAP_ID = HEAPS_COUNT + 1; //идентификатор глобальной кучи
thread_local const size_t THREAD_ID = std::hash<std::thread::id>()(std::this_thread::get_id()) % HEAPS_COUNT;


/*
 * Перед каждым куском выделенной памяти будем хранить указатель на суперблок, в котором он лежит
 */
class WrapOnSuperBlockPointer
{
public:
    void* superBlockPointer;
};

//--------------------SuperBlock Definition-------------------------------

class SuperBlock
{
public:
    SuperBlock() {}
    SuperBlock(size_t sizeOfBlock_) :
            heapNumber(0),
            sizeOfBlock(sizeOfBlock_),
            sizeOfUsed(0),
            beginsOfBlocks(SUPERBLOCK_SIZE / sizeOfBlock_)
            //countOfFreeBlocks(SUPERBLOCK_SIZE / sizeOfBlock_)  - непонятный CE
    {
        countOfFreeBlocks = SUPERBLOCK_SIZE / sizeOfBlock_; // изначально 0
        curPtr = (char*)malloc(SUPERBLOCK_SIZE); //выделенная под суперблок память
        for (size_t i = 0;i < beginsOfBlocks.size();++i) { // инициализируем начала блоков
            beginsOfBlocks[i] = i * sizeOfBlock;
        }
    }
    ~SuperBlock()
    {
        free(curPtr);
    }
    bool isFull();
    size_t getUsedMemory(); // получает количество используемой памяти
    size_t getSizeOfBlock(); // получает размера блока в данном суперблоке

    void* allocBlock();
    void deallocBlock(void* blockPtr);

    size_t heapNumber; //номер кучи, в которой лежит блок

    friend class Allocator;
private:
    char* curPtr; //выделенная память

    size_t countOfFreeBlocks; // количество свободных блоков.
                            // В ячейку с этим номером и будем писать
    size_t sizeOfBlock; // размер одного блока
    size_t sizeOfUsed; // размер используемой памяти

    std::vector<size_t> beginsOfBlocks;//вектор указателей на начала свободных блоков
};


//--------------------SuperBlock Implementation----------------


bool SuperBlock::isFull()
{
    return (countOfFreeBlocks == 0);
}

size_t SuperBlock::getUsedMemory()
{
    return sizeOfUsed;
}

size_t SuperBlock::getSizeOfBlock()
{
    return sizeOfBlock;
}
/*
 * Занимаем блоки с конца
 */
void* SuperBlock::allocBlock()
{
    if (!isFull()) {
        return (curPtr + beginsOfBlocks[--countOfFreeBlocks]);
    }
    return nullptr;
}

/*
 * Решили освободить блок по указателю
 * добавили новый свободный блок, изменили его указатель
 */
void SuperBlock::deallocBlock(void* blockPtr)
{
    char* memPtr = reinterpret_cast<char*>(blockPtr);
    beginsOfBlocks[countOfFreeBlocks++] = memPtr - curPtr;
}

//--------------------Basket Definiton----------------------------------


/*
 * Класс - корзина. В корзине лежат суперблоки, разбитые на одинаковые блоки
 */
class Basket
{
public:
    Basket() :
        sizeOfAllocated(0),
        sizeOfUsed(0)
    {}
    ~Basket()
    {
        /*
         * удаляем суперблоки
         */
        for (size_t i = 0;i < occupiedSuperBlocks.size();++i) {
            SuperBlock* superBlock = occupiedSuperBlocks[i];
            delete superBlock;
        }
        for (size_t i = 0;i < unoccupiedSuperBlocks.size();++i) {
            SuperBlock* superBlock = unoccupiedSuperBlocks[i];
            delete superBlock;
        }
        occupiedSuperBlocks.clear();
        unoccupiedSuperBlocks.clear();
    }

    void addSuperBlock(SuperBlock* superBlock); // добавить суперблок (не обязательно свободный)
    void deallocSuperBlock(SuperBlock* superBlock, void* ptr); // очистить память в суперблоке по указателю

    SuperBlock* getSuperBlock(); // получить произвольный не занятый суперблок
    std::pair<SuperBlock*, void*> getBlock(); // получить какой-то свободный блок

    friend class Allocator;
private:
    std::vector<SuperBlock*> occupiedSuperBlocks; // полностью занятые суперблоки
    std::vector<SuperBlock*> unoccupiedSuperBlocks; // не полностью занятые суперблоки

    std::size_t sizeOfAllocated; // суммарная память аллокированных
                                // суперблоко
    std::size_t sizeOfUsed; // сумма используемой памяти по всем
                            // суперблокам в корзине
};


//-------------------Basket Implementation------------------

/*
 * Добавляем суперблок в соответствующий список в зависимости от заполнения суперблока
 */
void Basket::addSuperBlock(SuperBlock *superBlock)
{
    if (superBlock->isFull()) {
        occupiedSuperBlocks.push_back(superBlock);
    } else {
        unoccupiedSuperBlocks.push_back(superBlock);
    }
}

/*
 * Получаем произвольный не полностью заполненный суперблок
 */
SuperBlock* Basket::getSuperBlock()
{
    if (unoccupiedSuperBlocks.size() == 0) {
        return nullptr;
    }
    SuperBlock* superBlock = unoccupiedSuperBlocks.back();
    unoccupiedSuperBlocks.pop_back();
    return superBlock;
}


/*
 * Хотим удалить некоторый блок, который лежит в данном суперблоке
 * Проверяем, заполнен он или нет, чтобы узнать, в каком из списков находится
 * Если в заполненных, то освобождаем память, переносим суперблок в список незаполненных
 */
void Basket::deallocSuperBlock(SuperBlock *superBlock, void *ptr)
{
    if (!(superBlock->isFull())) {
        superBlock->deallocBlock(ptr);
        return;
    }
    superBlock->deallocBlock(ptr);
    for (size_t i = 0;i < occupiedSuperBlocks.size();++i) {
        if (occupiedSuperBlocks[i] == superBlock) {
            std::swap(occupiedSuperBlocks[i], occupiedSuperBlocks.back());
            occupiedSuperBlocks.pop_back();
            unoccupiedSuperBlocks.push_back(superBlock);
            return;
        }
    }
}

/*
 * Возвращает пару из указателя на суперблок и выделенного в нём блока
 * Удаляет из корзины суперблок
 */
std::pair<SuperBlock*, void*> Basket::getBlock()
{
    if (unoccupiedSuperBlocks.size() == 0) {
        return {nullptr, nullptr};
    }
    SuperBlock* superBlock = unoccupiedSuperBlocks.back();
    unoccupiedSuperBlocks.pop_back();
    void* block = superBlock->allocBlock();
    return {superBlock, block};
};


//--------------------Heap Definition----------------------------

class Heap
{
public:
    Heap() :
            sizeOfLargestBasket(minBasketSize)
    {
        while (sizeOfLargestBasket <= SUPERBLOCK_SIZE / 2) { // блоки большего размера будут храниться в глобальной куче
            baskets.push_back(Basket()); // в куче лежат корзины разных размеров(по степеням двойки)
            sizeOfLargestBasket *= 2; // в данном случае, степени двойки
        }
    }
    ~Heap()
    {

    }

    size_t getSuitableBasketSize(size_t memSize); // "округлить" размер memSize до размера блока
    size_t getBasketNumber(size_t basketSize); // получить номер корзины, подходящей под данный basketSize
    Basket* getBasket (size_t memSize); // получить корзину, подходящую

    std::mutex heapMutex;
private:
    size_t minBasketSize = 16; // размер минимальных блоков в куче
    size_t sizeOfLargestBasket; //наибольший размер

    std::vector<Basket> baskets; // вектор корзин
};



//--------------------Heap Implementation------------------------

/*
 * Пробегаем от начала до тех пор, пока не встретим корзину подходящего размера
 * возвращаем размер этой корзины
 */
size_t Heap::getSuitableBasketSize(size_t memSize)
{
    size_t ind = 0;
    size_t curSize = minBasketSize;
    while (curSize < memSize) {
        ++ind;
        curSize *= 2;
    }
    return curSize;
}

/*
 * пробегаем от начала до тех пор, пока не встретим корзину подходящего размера
 * возвращаем номер этой корзины
 */
size_t Heap::getBasketNumber(size_t basketSize)
{
    size_t ind = 0;
    size_t curSize = minBasketSize;
    while (curSize < basketSize) {
        ++ind;
        curSize *= 2;
    }
    return ind;
}

/*
 * Получаем указатель на корзину нужного размера
 */
Basket* Heap::getBasket(size_t memSize)
{
    return &(baskets[getBasketNumber(memSize)]);
}

//--------------------Allocator Definition----------------------------

class Allocator
{
public:
    Allocator()
    {
        offset = sizeof(WrapOnSuperBlockPointer);
        grandHeap = new Heap();
        for (size_t i = 0;i < HEAPS_COUNT;++i) {
            heaps.push_back(new Heap());
        }
    }
    ~Allocator()
    {
        for (size_t i = 0;i < HEAPS_COUNT;++i) {
            delete heaps[i];
        }
        heaps.clear();

        delete grandHeap;
    }
    void* allocate(size_t bytes);
    void deallocate(void* ptr);
private:
    Heap* grandHeap; // "глобальная" куча
    std::vector<Heap*> heaps; // все остальный кучи.
    size_t offset; // размер сдвига SuperBlock, для того, чтобы перед ним поместить указатель на кучу
};

//--------------------Allocator Implementation------------------------

void* Allocator::allocate(size_t bytes)
{
    //если размер блока слишком велик, то целесообразно выделять его в глобальной куче
    if (bytes >= SUPERBLOCK_SIZE / 2) {
        void* ptr = malloc(bytes + offset);
        if (ptr == nullptr) {
            return nullptr;
        }
        /*SuperBlock* superBlockPointer = reinterpret_cast<SuperBlock*>(ptr);
        superBlockPointer = nullptr;
        return reinterpret_cast<char*>(ptr) + offset;*/
        WrapOnSuperBlockPointer* pointer = reinterpret_cast<WrapOnSuperBlockPointer*>(ptr);
        pointer->superBlockPointer = nullptr;
        return reinterpret_cast<char*>(ptr) + offset;
    }
    void* resultPtr = nullptr; // ответ
    size_t heapNumber = THREAD_ID;// определили кучу для текущего потока
    Heap* heap = heaps[heapNumber];
    std::unique_lock<std::mutex> heapLock(heap->heapMutex);//заблокировали её
    Basket* basket = heap->getBasket(bytes + offset);//выбрали подходящий basket, чтобы всё влезло
    std::pair<SuperBlock*, void*> block = basket->getBlock();//получили блок
    resultPtr = block.second;
    SuperBlock* currentSuperBlock = block.first; // здесь и будет записан ответ
    if (currentSuperBlock == nullptr) { // nullptr - значит, нет свободного суперблока в данной корзине
        std::unique_lock<std::mutex> grandHeapLock(grandHeap->heapMutex); // блокируем кучу
        Basket* grandHeapBasket = grandHeap->getBasket(bytes + offset); //находим подходящую корзину в глобальной куче
        std::pair<SuperBlock*, void*> grandHeapBlock = grandHeapBasket->getBlock(); // взяли блок из глобальной кучи
        SuperBlock* grandHeapSuperBlock = grandHeapBlock.first;
        if (grandHeapSuperBlock == nullptr) { // если и тут нет свободного суперблока, то
                                                // добавляем новый суперблок в текущую кучу
            currentSuperBlock = new SuperBlock(heap->getSuitableBasketSize(bytes + offset));
            currentSuperBlock->heapNumber = heapNumber;
            resultPtr = currentSuperBlock->allocBlock();
            basket->sizeOfAllocated += SUPERBLOCK_SIZE;
        } else {
            resultPtr = grandHeapBlock.second; // если же нашёлся свободный суперблок, то
            currentSuperBlock = grandHeapSuperBlock; // добавляем суперблок в глобальную кучу
            currentSuperBlock->heapNumber = heapNumber;

            grandHeapBasket->sizeOfAllocated -= SUPERBLOCK_SIZE; // поправляем информацию о выделенной и
            basket->sizeOfAllocated += SUPERBLOCK_SIZE; // используемой памяти
            grandHeapBasket->sizeOfUsed -= currentSuperBlock->getUsedMemory();
            basket->sizeOfUsed += currentSuperBlock->getUsedMemory();
        }
    }

    char* bytesPtr = reinterpret_cast<char*>(resultPtr);
    /*SuperBlock* superBlockMaster = reinterpret_cast<SuperBlock*>(bytesPtr);
    superBlockMaster = currentSuperBlock;*/
    WrapOnSuperBlockPointer* wrapOnSuperBlockPointer = reinterpret_cast<WrapOnSuperBlockPointer*> (bytesPtr);
                                                    //записываем указатель на суперблок перед выделяемой памятью
    wrapOnSuperBlockPointer->superBlockPointer = currentSuperBlock;
    currentSuperBlock->sizeOfUsed += currentSuperBlock->getSizeOfBlock();
    basket->sizeOfUsed += currentSuperBlock->getSizeOfBlock(); // пересчитываем память
    basket->addSuperBlock(currentSuperBlock);//добавляем суперблок в корзину
    resultPtr = bytesPtr + offset;//возвращаем результат
    return resultPtr;
}

void Allocator::deallocate(void *ptr)
{
    /*if (ptr == nullptr) {
        return;
    }*/
    assert(ptr != nullptr);//не стоит деаллоцировать память, которую не выделяли
    char* bytesPtr = reinterpret_cast<char*>(ptr) - offset;
    WrapOnSuperBlockPointer* wrapOnSuperBlockPointer = reinterpret_cast<WrapOnSuperBlockPointer*> (bytesPtr);
    SuperBlock* superBlock = reinterpret_cast<SuperBlock*>(wrapOnSuperBlockPointer->superBlockPointer);
                                        //получаем информацию о суперблоке
    //SuperBlock* superBlock = reinterpret_cast<SuperBlock*>(bytesPtr);
    if (superBlock == nullptr) { // данные лежат в глобальной куче. Освобождаем так же, как и аллоцировали
        free(bytesPtr);
        return;
    }
    size_t heapNumber = GRAND_HEAP_ID; //ищем кучу
    Heap* heap = grandHeap;
    size_t count = 0; // нужно, чтобы хотя бы раз цикл выполнился
    while (count == 0 || heapNumber != superBlock->heapNumber) { // пока не найдём
        if (count > 0) {
            if (heapNumber == GRAND_HEAP_ID) {
                grandHeap->heapMutex.unlock();
            } else {
                heaps[heapNumber]->heapMutex.unlock();
            }
        }
        heapNumber = superBlock->heapNumber;
        if (heapNumber == GRAND_HEAP_ID) { // находим кучу, в которой лежит суперблок
            grandHeap->heapMutex.lock();
            heap = grandHeap;
        } else {
            heaps[heapNumber]->heapMutex.lock();
            heap = heaps[heapNumber];
        }
        ++count;
    }
    Basket* basket = heap->getBasket(superBlock->getSizeOfBlock());//нашли кучу, откуда деаллоцировать
    superBlock->sizeOfUsed -= superBlock->getSizeOfBlock();
    basket->sizeOfUsed -= superBlock->getSizeOfBlock();
    basket->deallocSuperBlock(superBlock, bytesPtr); // деаллоцировали память
    if (heapNumber == GRAND_HEAP_ID) {
        grandHeap->heapMutex.unlock(); //если глобальная куча, то деаллоцировали
        return;
    }
    if ((basket->sizeOfUsed < basket->sizeOfAllocated - 4 * SUPERBLOCK_SIZE) && // если же нет
            (basket->sizeOfUsed * 4 < 3 * basket->sizeOfAllocated)) { // проверяем, не выгодно ли нам  перенести
        //std::unique_lock<std::mutex> grandBasketLock(grandHeap->heapMutex); // блок в глобальную кучу. Если выгодно, то
        Basket* grandBasket = grandHeap->getBasket(superBlock->getSizeOfBlock()); // переносим
        SuperBlock* currentSuperBlock = basket->getSuperBlock();
        currentSuperBlock->heapNumber = GRAND_HEAP_ID;

        grandBasket->sizeOfUsed += currentSuperBlock->getUsedMemory();// аккуратно пересчитываем память
        basket->sizeOfUsed -= currentSuperBlock->getUsedMemory();
        grandBasket->sizeOfAllocated += SUPERBLOCK_SIZE;
        basket->sizeOfAllocated -= SUPERBLOCK_SIZE;

        grandBasket->addSuperBlock(currentSuperBlock); // разблокируем
    }
    heap->heapMutex.unlock();
}


Allocator allocator;

extern void* mtalloc(size_t bytes)
{
    return allocator.allocate(bytes);
}

extern void mtfree(void* ptr)
{
    allocator.deallocate(ptr);
}

/*int main ()
{
    char* xxx = (char*)mtalloc(4);
    mtfree(xxx);
    return 0;
}*/
