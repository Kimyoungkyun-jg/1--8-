# 물리 튜닝 값 정리

값을 바꾸기 전에 **증상 → 손잡이** 표(맨 아래)를 먼저 보면 빠릅니다.

---

## 1. 솔버 — 실행 중 조절 가능

전부 `CollisionManager` 클래스의 public 멤버라, **Physics Debug 창의 Solver / Sleep 항목에서 슬라이더로 실시간 조절**할 수 있습니다. 여기서 찾은 값을 헤더의 기본값에 반영하면 됩니다.

| 변수 | 선언 위치 | 현재 값 | 의미 |
|---|---|---|---|
| `velocityIterations` | `CollisionManager.h` · `CollisionManager` | `8` | 충격량이 접촉을 타고 전파되는 횟수. 반복 1회 ≈ 접촉 1개만큼 전파되므로 **높이 쌓을수록 더 필요** |
| `positionIterations` | 〃 | `15` | 겹침 해소 반복 횟수 |
| `baumgarte` | 〃 | `0.8` | 한 번의 위치 반복에서 초과 침투의 몇 %를 지울지. N회 후 대략 `(1-β)^N`만 남음 |
| `slop` | 〃 | `0.0005` | 일부러 허용하는 침투량. 0으로 두면 매 프레임 밀어내고 중력이 다시 밀어넣어 **지터** 발생 |
| `rollingResistance` | 〃 | `0.002` | 구름 저항의 팔 길이. **무차원이 아니라 길이 단위** (저항 토크 = 법선 하중 × 이 값) |
| `CollisionThreshold` | 〃 | `50.0` | 데미지가 들어가는 충격량 문턱. **지지 하중을 뺀 순수 충돌분**과 비교 |
| `bWarmStarting` | 〃 | `true` | 지난 프레임 충격량 이월. 끄면 탑이 가라앉고 미끄러짐 |
| `bSleepEnabled` | 〃 | `true` | 슬립 전체 on/off |
| `linearSleepTolerance` | 〃 | `0.015` | 이보다 느리면 정지로 간주 |
| `angularSleepTolerance` | 〃 | `0.035` | 각속도 기준. 약 2°/초 |
| `timeToSleep` | 〃 | `0.5` | 이 시간(초)만큼 느린 상태가 이어져야 잠듦 |

> **무리(island) 단위 판정**이라, 연결된 물체 중 **하나라도** 임계값을 못 넘으면 그 무리 전체가 안 잠듭니다. 안 잠들면 Physics Debug의 물체별 목록에서 `ok`가 안 붙는 ID를 찾으세요.

---

## 2. 솔버 내부 상수 — 코드에 박혀 있음

슬라이더가 없습니다. 바꾸려면 코드를 고쳐야 합니다.

| 변수 | 선언 위치 | 현재 값 | 의미 |
|---|---|---|---|
| `relativeTolerance` | `CollisionManager.cpp` · `OverlapOBB()` | `0.95` | 기준면 이력(hysteresis). b축이 이만큼 더 얕아야 기준을 b로 넘김. 낮추면 기준이 자주 뒤집혀 **접촉 ID가 흔들림** |
| `absoluteTolerance` | 〃 | `0.0001` | 위와 함께 쓰는 절대 여유 |
| `contactTolerance` | 〃 | `0.005` | 기준면에서 이만큼 떨어진 점까지 접촉으로 유지. **접촉점 개수를 안정시키는 값** |
| `restitutionThreshold` | `CollisionManager.cpp` · `InitContact()` | `1.0` | 이보다 느린 충돌은 반발계수를 0으로 죽임 (놓인 물체 떨림 방지) |
| `MinDamageSpeed` | `CollisionManager.cpp` · `CheckCollisionAll()` | `0.1` | 접근 속도가 이보다 느리면 데미지 없음 |

---

## 3. 물체 물성 — 기본값

`ACollider`의 protected 멤버입니다. 아래 클래스들이 생성자에서 덮어씁니다.

| 변수 | 선언 위치 | 기본값 | 의미 |
|---|---|---|---|
| `Mass` | `UObject.h` · `ACollider` | `10` | 질량. **0이면 정적**(벽처럼 안 움직이고 솔버가 안 밀어냄) |
| `StaticFriction` | 〃 | `0.5` | 정지 마찰. 쌍 조합은 **기하평균** `sqrt(a*b)` |
| `DynamicFriction` | 〃 | `0.3` | 운동 마찰. 정지 한계를 넘으면 이 값으로 클램프 |
| `Restitution` | 〃 | `0.2` | 반발계수. 쌍 조합은 **큰 쪽** `max(a, b)` — 잘 튀는 물체가 이김 |
| `LinearDamping` | 〃 | `0.0` | 선속도 감쇠 |
| `AngularDamping` | 〃 | `2.0` | 각속도 감쇠 |
| `hp` | 〃 | `1.0` | 체력. 0이 되면 파괴 |

> 마찰과 반발의 **조합 규칙이 다릅니다.** 마찰은 둘 다 미끄러워야 미끄럽고, 반발은 한쪽만 잘 튀어도 잘 튑니다.

---

## 4. 클래스별 재정의

각 클래스 생성자(`UObject.h`)에서 잡습니다.

| 클래스 | 정지 마찰 | 운동 마찰 | 반발 | 의도 |
|---|---|---|---|---|
| `ABird` | `0.4` | `0.3` | `0.4` | 잘 구르고 잘 튐 |
| `APig` | `0.5` | `0.4` | `0.2` | 중간 |
| `ABlock` | `0.6` | `0.5` | `0.05` | 나무. 거의 안 튀고 잘 안 미끄러짐 |
| `AGround` | `0.7` | `0.6` | `0.1` | 바닥·벽. 제일 잘 잡아줌 |

---

## 5. 월드 · 프레임

| 변수 | 선언 위치 | 현재 값 | 의미 |
|---|---|---|---|
| `Global::G` | `Global.h` | `(0, -4.9, 0)` | 중력 가속도 |
| `Global::topBorder` | 〃 | `1.0` | 화면 위 경계 |
| `Global::bottomBorder` | 〃 | `-1.0` | 화면 아래 경계 |
| `thickness` | `GameManager.cpp` · `SpawnWalls()` | `0.5` | 벽 두께. 빠른 물체가 한 스텝에 통과하지 않을 만큼 필요 |
| `targetFPS` | `main.cpp` · `WinMain()` | `144` | 목표 프레임 |
| `fixedDeltaTime` | 〃 | `1000/144` ms | **물리 한 스텝의 크기.** 프레임과 무관하게 항상 이만큼만 진행 |
| `maxAccumulated` | 〃 | `fixedDeltaTime × 5` | 한 프레임에 갚을 수 있는 최대 물리 시간. 넘으면 버려서 악순환을 막음 |

> **가로 경계는 상수가 아닙니다.** `URenderer::wAspectRatio`(스왑체인 종횡비)에서 가져옵니다 — 셰이더가 x를 이 값으로 나누므로 보이는 x 범위가 곧 `±wAspectRatio`입니다.

---

## 6. 게임플레이

| 변수 | 선언 위치 | 현재 값 | 의미 |
|---|---|---|---|
| `ASlingShot::Power` | `UObject.h` · `ASlingShot` | `10.0` | 발사 세기 배수. 속도 = (당긴 벡터) × 이 값 |
| `ABird::CanStretcheLength` | `UObject.h` · `ABird` | `0.6` | 새총 최대 당김 거리. **최대 발사 속도 = 0.6 × Power = 6.0** |
| `ABand::k` | `UObject.h` · `ABand` | `300` | 밴드 스프링 강성 |
| `ABand::c` | 〃 | `8` | 밴드 스프링 감쇠 |

스폰 기본 크기·질량 (`main.cpp` Castle Editor / `GameManager.cpp`)

| 대상 | 크기 | 질량 |
|---|---|---|
| 블록 | `0.4 × 0.05` | `70` |
| 돼지 | `0.15 × 0.15` | `30` |
| 새 | `0.1 × 0.1` | `50` |

---

## 증상 → 손잡이

| 증상 | 먼저 볼 값 |
|---|---|
| 탑이 눌렸다 펴짐 (스펀지) | `baumgarte` ↑ → `positionIterations` ↑ |
| 블록이 서로 파고든 채 남음 | 같음. `max penetration` 그래프가 `slop` 근처로 내려갈 때까지 |
| 미세하게 떨림 (지터) | `slop` ↑ |
| 접촉점 개수가 깜빡임 | `contactTolerance` ↑ |
| 탑이 옆으로 미끄러짐 | 클래스별 마찰 ↑ (`ABlock`, `AGround`) |
| 공이 경사에서 안 멈춤 | `rollingResistance` ↑ |
| 아무것도 안 잠듦 | `linearSleepTolerance` ↑, `timeToSleep` ↓ |
| 움직이는 중인데 굳어버림 | 위 값들을 반대로 ↓ |
| 너무 잘 깨짐 / 안 깨짐 | `CollisionThreshold` |
| 물체가 벽을 통과함 | `thickness` ↑, 또는 `fixedDeltaTime` ↓ |
| 충돌 순간 반응이 약함 | `velocityIterations` ↑ |

---

## 측정 계기

Physics Debug 창에서 값 대신 **숫자를 보고** 맞출 수 있습니다.

- **`max penetration`** — 위치 보정 후 남은 겹침. `slop` 근처에서 평평하면 수렴. 톱니면 진동, 우상향이면 발산
- **`kinetic energy`** — 전체 운동에너지. 잦아들면 정상
- **접촉 목록** — 접촉점별 `id` / `pen` / `Pn` / `Pt`. 정지한 물체에서 `id`가 바뀌면 warm starting이 작동하지 않는 것
- **물체 목록** — 물체별 속도·각속도·슬립 타이머. 무리가 안 잠들 때 범인을 찾는 용도
- **`slip`** — 원이 접촉점에서 실제로 미끄러지는 속도. 0에 가까우면 구르는 중이라 **마찰이 관여할 수 없음**
