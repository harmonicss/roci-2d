#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <map>
#include <queue>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <iostream>

using Entity = std::uint32_t;
constexpr Entity MAX_ENTITIES = 5000;

///////////////////////////////////////////////////////////////////////////////
// ENTITY MANAGER
///////////////////////////////////////////////////////////////////////////////
class EntityManager {
  std::queue<Entity> freeEntities;
  std::array<bool, MAX_ENTITIES> alive{};
  std::map<Entity, std::string> name{}; // e.g. Rocinante, enemy, bullet

  // use unordered_map for faster access for all entities with a given name
  // could have std::vector<Entity> instead of int, keep simple for now
  std::unordered_multimap<std::string, int> nameToIds{};

public:
  EntityManager() {
    for (Entity e = 0; e < MAX_ENTITIES; ++e) {
      freeEntities.push(e);
    }
  }

  Entity create(std::string ename = "") {
    assert(!freeEntities.empty() && "Too many entities created");
    Entity id = freeEntities.front();
    freeEntities.pop();
    alive[id] = true;
    name.insert({id, ename});
    nameToIds.insert({ename, id});
    return id;
  }

  void destroy(Entity e) {
    assert(alive[e] && "Destroying non existant entity");
    alive[e] = false;
    freeEntities.push(e);

    // erase the entity with name i.e a single bullet from the unordered_multimap
    const std::string ename = name.at(e);
    auto range = nameToIds.equal_range(ename);
    for (auto it = range.first; it != range.second;) {
      if (it->second == e) {
        // erase returns the next iterator; safe during iteration
        // shouldnt need this as we break, but just in case. 
        it = nameToIds.erase(it);
        break;
      }
      else {
        ++it;
      }
    }

    name.erase(e);
  }

  bool isAlive(Entity e) {
    assert(e < MAX_ENTITIES && "Entity out of range");
    return alive[e];
  }

  std::string getName(Entity e) {
    assert(e < MAX_ENTITIES && "Entity out of range");
    return name.at(e);
  }

  // e.g. get all Missiles
  std::vector<Entity> getEntitiesByName(std::string ename) {
    std::vector<Entity> result;
    auto range = nameToIds.equal_range(ename);

    for (auto it = range.first; it != range.second; ++it) {
      result.push_back(it->second);
    }
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
// COMPONENT MANAGER
///////////////////////////////////////////////////////////////////////////////

// I for interface, polymorphic destructor: correct derived-class destructor
// will be called
class IComponentArray {
public:
  virtual ~IComponentArray() = default;
};

template <typename T> class ComponentArray : public IComponentArray {
  std::unordered_map<Entity, T> data;

public:
  // insert_or_assign will either insert a new element by moving in the
  // component, or overwrite an existing one. This prevents the need for a
  // default constructor in SpriteComponent.
  void insert(Entity e, T component) {
    data.insert_or_assign(e, std::move(component));
  }
  void remove(Entity e) { data.erase(e); }
  T &get(Entity e) { return data.at(e); }
  bool has(Entity e) { return data.find(e) != data.end(); }

  std::vector<Entity> getEntities() const {
    std::vector<Entity> v;
    v.reserve(data.size());
    for (auto const &kv : data) {
      v.push_back(kv.first);
    }
    return v;
  }
};

class ComponentManager {
  std::unordered_map<std::type_index, IComponentArray *> componentArrays;

public:
  ~ComponentManager() {
    for (auto &kv : componentArrays) {
      delete kv.second;
    }
  }

  template <typename T> void registerComponent() {
    componentArrays[typeid(T)] = new ComponentArray<T>();
  }

  template <typename T> void addComponent(Entity e, T component) {
    getArray<T>()->insert(e, std::move(component));
  }

  template <typename T> void removeComponent(Entity e) {
    getArray<T>()->remove(e);
  }

  template <typename T> T &getComponent(Entity e) {
    return getArray<T>()->get(e);
  }

  template <typename T> bool hasComponent(Entity e) {
    return getArray<T>()->has(e);
  }

  template <typename T> ComponentArray<T> *getArray() {
    // `it` is an iterator. It “points” to the map entry whose key is
    // type_index(T).
    auto it = componentArrays.find(typeid(T));
    assert(it != componentArrays.end() && "Component not registered");

    // return the ComponentArray<T> pointer
    return static_cast<ComponentArray<T> *>(it->second);
  }

  // returns a std::array of IComponentArray* (or ComponentArray<Base>*)
  // one for each Comps in the pack
  template <typename... Comps> auto getArrays() {
    return std::array<IComponentArray *, sizeof...(Comps)>{
        getArray<Comps>()...};
  }
};

///////////////////////////////////////////////////////////////////////////////
// Coordinator (Facade)
///////////////////////////////////////////////////////////////////////////////
class Coordinator {
  EntityManager entityMgr;
  ComponentManager compMgr;

public:
  Entity createEntity() { return entityMgr.create(); }
  Entity createEntity(std::string name) { return entityMgr.create(name); }
  void destroyEntity(Entity e) { entityMgr.destroy(e); }

  template <typename T> void registerComponent() {
    compMgr.registerComponent<T>();
  }

  template <typename T> void addComponent(Entity e, T comp) {
    compMgr.addComponent<T>(e, std::move(comp));
  }

  template <typename T> void removeComponent(Entity e) {
    compMgr.removeComponent<T>(e);
  }

  template <typename T> T &getComponent(Entity e) {
    return compMgr.getComponent<T>(e);
  }

  template <typename T> bool hasComponent(Entity e) {
    return compMgr.hasComponent<T>(e);
  }

  std::string getEntityName(Entity e) { return entityMgr.getName(e); }

  bool isAlive(Entity e) { return entityMgr.isAlive(e); }

  // View: get all entities with ALL of the listed components
  // Variadic template declaration
  template <typename First, typename... Rest> std::vector<Entity> view() {

    auto *firstArr = compMgr.getArray<First>();
    auto list = firstArr->getEntities();

    std::vector<Entity> result;
    result.reserve(list.size());

    for (Entity e : list) {
      // check the rest of the pack of components
      if ((compMgr.getArray<Rest>()->has(e) && ...)) {
        result.push_back(e);
      }
    }
    return result;
  }

  // get all entities that have the given name i.e. all Bullets or Torpedos
  std::vector<Entity> getEntitiesByName(std::string ename) {
    return entityMgr.getEntitiesByName(ename);
  }
};
