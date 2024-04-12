//carregamento das bibliotecas a serem utilizadas:
#include <Fuzzy.h>
#include <Arduino.h>
#include <FuzzyRuleAntecedent.h>
#include <stdlib.h>
#include <FuzzySet.h>
#include <FuzzyRuleAntecedent.h>
#include <FuzzyRuleConsequent.h>
#include <inttypes.h>
#include <FuzzyInput.h>
#include <FuzzyOutput.h>
#include <FuzzyRule.h>
#include <FuzzyComposition.h>
#include <FuzzyIO.h>
#include <stdio.h>
#include <time.h>
#include "DHT.h"

//nomeação dos pinos do hardware ESP32
#define O_IRRIGA 32 //variável de saída fuzzy: fechamento do relé e acionamento da irrigação. Pino 32
#define I_TEMPUMI 4 //variável de entrada 1 fuzzy: sensor de temperatura/umidade: um só sensor fornecerá dados para duas variáveis de entrada: temperatura e umidade. Pino 4
#define I_UMISOLO 34 //variável de entrada 2 fuzzy: sensor de umidade do solo. Pino 34
#define DHTTYPE DHT11

DHT dht(I_TEMPUMI, DHTTYPE);   //inicializando o sensor. 

Fuzzy *fuzzy = new Fuzzy();

//Definição dos termos linguísticos como funções de pertinência trapezoidal.
  FuzzySet *frio = new FuzzySet(0, 0, 6, 15);
  FuzzySet *moderada = new FuzzySet(6, 12, 22, 30);
  FuzzySet *quente = new FuzzySet(24, 30, 40, 40); 
  FuzzySet *pouco_umido = new FuzzySet(0, 0, 30, 45);
  FuzzySet *adequado = new FuzzySet(30, 40, 60, 70);
  FuzzySet *muito_umido = new FuzzySet(55, 70, 100, 100);
  FuzzySet *seco = new FuzzySet(0, 0, 60, 80);
  FuzzySet *umido = new FuzzySet(60, 80, 90, 90);
  FuzzySet *nenhum = new FuzzySet(0, 0, 5, 40);
  FuzzySet *curto = new FuzzySet(10, 55, 65, 120);
  FuzzySet *medio = new FuzzySet(80, 155, 165, 240);
  FuzzySet *longo = new FuzzySet(180, 255, 265, 340);
  FuzzySet *maxi = new FuzzySet(300, 380, 400, 400);

void setup()
{
  //CONFIGURAÇÃO DE CADA PINO DEFINIDO ANTERIORMENTE PARA FUNCIONAREM COMO ENTRADAS OU SAÍDAS
  pinMode(O_IRRIGA,OUTPUT);
  pinMode(I_TEMPUMI,INPUT);
  pinMode(I_UMISOLO,INPUT);

  // INICIALIZAÇÃO DA TRANSMISSÃO SERIAL NA TAXA DE 9600 bits por segundo
  Serial.begin(9600);
  Serial.println(F("Teste do DHT11!"));
  dht.begin();

  // CRIAÇÃO DA VARIÁVEL LINGUÍSTICA DE ENTRADA TEMPERATURA e atribuição como entrada 1 da lógica fuzzy
  FuzzyInput *temperatura = new FuzzyInput(1);
  // Inclusão dos termos linguísticos na variável linguística, com os parâmetros definidos anteriormente.
  temperatura->addFuzzySet(frio);
  temperatura->addFuzzySet(moderada);
  temperatura->addFuzzySet(quente);
  // Atribuição da variável linguística criada como entrada do sistema fuzzy.
  fuzzy->addFuzzyInput(temperatura);

  // CRIAÇÃO DA VARIÁVEL LINGUÍSTICA DE ENTRADA UMIDADE DO SOLO:
  FuzzyInput *umidade_solo = new FuzzyInput(2);

  umidade_solo->addFuzzySet(pouco_umido);
  umidade_solo->addFuzzySet(adequado);
  umidade_solo->addFuzzySet(muito_umido);

  fuzzy->addFuzzyInput(umidade_solo);

  // CRIAÇÃO DA VARIÁVEL LINGUÍSTICA DE ENTRADA UMIDADE DO AR:
  FuzzyInput *umidade_ar = new FuzzyInput(3);

  umidade_ar->addFuzzySet(seco);
  umidade_ar->addFuzzySet(umido);

  fuzzy->addFuzzyInput(umidade_ar);

// CRIAÇÃO DA VARIÁVEL LINGUÍSTICA DE SAÍDA TEMPO DE IRRIGAÇÃO:
  FuzzyOutput *tempo_irrigacao = new FuzzyOutput(1);

  tempo_irrigacao->addFuzzySet(nenhum);
  tempo_irrigacao->addFuzzySet(curto);
  tempo_irrigacao->addFuzzySet(medio);
  tempo_irrigacao->addFuzzySet(longo);
  tempo_irrigacao->addFuzzySet(maxi);

  fuzzy->addFuzzyOutput(tempo_irrigacao);

  // construindo a regra 1: Se umidade_solo = pouco_umido AND umidade_ar = seco AND temperatura = frio THEN tempo_irrigação = medio

  // instanciando objetos antecedentes da regra fuzzy
  FuzzyRuleAntecedent *if_pouco_umido_seco_frio = new FuzzyRuleAntecedent();
  // criando um antecedente de regra fuzzy usando um JOIN With AND entre 3 objetos (alterado da biblioteca padrão)
  if_pouco_umido_seco_frio->joinWithAND(pouco_umido, seco, frio);
  // declaração do consequente a ser usado com essa regra. Sò é necessário caso não tenha sido declarado ainda. em regra anterior.
  FuzzyRuleConsequent *entao_tempo_medio = new FuzzyRuleConsequent();
  // Atribuindo um valor fuzzy ao consequente criado anteriormente
  entao_tempo_medio->addOutput(medio);
  // instanciando regra fuzzy por meio dos argumentos 
  FuzzyRule *R_poucoumido_seco_frio = new FuzzyRule(1, if_pouco_umido_seco_frio, entao_tempo_medio);
  // incluindo a regra fuzzy
  fuzzy->addFuzzyRule(R_poucoumido_seco_frio);

// construindo a regra 2: Se umidade_solo = pouco_umido AND umidade_ar = seco AND temperatura = moderada THEN tempo_irrigação = maximo

  FuzzyRuleAntecedent *if_pouco_umido_seco_moderada = new FuzzyRuleAntecedent();
  if_pouco_umido_seco_moderada->joinWithAND(pouco_umido, seco, moderada);
  FuzzyRuleConsequent *entao_tempo_maximo = new FuzzyRuleConsequent();
  entao_tempo_maximo->addOutput(maxi);
  FuzzyRule *R_poucoumido_seco_moderada = new FuzzyRule(2, if_pouco_umido_seco_moderada, entao_tempo_maximo);
  fuzzy->addFuzzyRule(R_poucoumido_seco_moderada);

// construindo a regra 3: Se umidade_solo = pouco_umido AND umidade_ar = seco AND temperatura = quente THEN tempo_irrigação = maximo

  FuzzyRuleAntecedent *if_pouco_umido_seco_quente = new FuzzyRuleAntecedent();
  if_pouco_umido_seco_quente->joinWithAND(pouco_umido, seco, quente);
  FuzzyRule *R_poucoumido_seco_quente = new FuzzyRule(3, if_pouco_umido_seco_quente, entao_tempo_maximo);
  fuzzy->addFuzzyRule(R_poucoumido_seco_quente);

// construindo a regra 4: Se umidade_solo = pouco_umido AND umidade_ar = umido AND temperatura = frio THEN tempo_irrigação = médio

  FuzzyRuleAntecedent *if_pouco_umido_umido_frio = new FuzzyRuleAntecedent();
  if_pouco_umido_umido_frio->joinWithAND(pouco_umido, umido, frio);
  FuzzyRule *R_poucoumido_umido_frio = new FuzzyRule(4, if_pouco_umido_umido_frio, entao_tempo_medio);
  fuzzy->addFuzzyRule(R_poucoumido_umido_frio);

// construindo a regra 5: Se umidade_solo = pouco_umido AND umidade_ar = umido AND temperatura = moderada THEN tempo_irrigação = longo

  FuzzyRuleAntecedent *if_pouco_umido_umido_moderada = new FuzzyRuleAntecedent();
  if_pouco_umido_umido_moderada->joinWithAND(pouco_umido, umido, moderada);
  FuzzyRuleConsequent *entao_tempo_longo = new FuzzyRuleConsequent();
  entao_tempo_longo->addOutput(longo);
  FuzzyRule *R_poucoumido_umido_moderada = new FuzzyRule(5, if_pouco_umido_umido_moderada, entao_tempo_longo);
  fuzzy->addFuzzyRule(R_poucoumido_umido_moderada);

// construindo a regra 6: Se umidade_solo = pouco_umido AND umidade_ar = umido AND temperatura = quente THEN tempo_irrigação = máximo

  FuzzyRuleAntecedent *if_pouco_umido_umido_quente = new FuzzyRuleAntecedent();
  if_pouco_umido_umido_quente->joinWithAND(pouco_umido, umido, quente);
  FuzzyRule *R_poucoumido_umido_quente = new FuzzyRule(6, if_pouco_umido_umido_quente, entao_tempo_maximo);
  fuzzy->addFuzzyRule(R_poucoumido_umido_quente);

// construindo a regra 7: Se umidade_solo = adequado AND umidade_ar = seco AND temperatura = frio THEN tempo_irrigação = nenhum

  FuzzyRuleAntecedent *if_adequado_seco_frio = new FuzzyRuleAntecedent();
  if_adequado_seco_frio->joinWithAND(adequado, seco, frio);
  FuzzyRuleConsequent *entao_tempo_nenhum = new FuzzyRuleConsequent();
  entao_tempo_nenhum->addOutput(nenhum);
  FuzzyRule *R_adequado_seco_frio = new FuzzyRule(7, if_adequado_seco_frio, entao_tempo_nenhum);
  fuzzy->addFuzzyRule(R_adequado_seco_frio);

// construindo a regra 8: Se umidade_solo = adequado AND umidade_ar = seco AND temperatura = moderada THEN tempo_irrigação = nenhum

  FuzzyRuleAntecedent *if_adequado_seco_moderada = new FuzzyRuleAntecedent();
  if_adequado_seco_moderada->joinWithAND(adequado, seco, moderada);
  FuzzyRule *R_adequado_seco_moderada = new FuzzyRule(8, if_adequado_seco_moderada, entao_tempo_nenhum);
  fuzzy->addFuzzyRule(R_adequado_seco_moderada);

// construindo a regra 9: Se umidade_solo = adequado AND umidade_ar = seco AND temperatura = quente THEN tempo_irrigação = curto

  FuzzyRuleAntecedent *if_adequado_seco_quente = new FuzzyRuleAntecedent();
  if_adequado_seco_quente->joinWithAND(adequado, seco, quente);
  FuzzyRuleConsequent *entao_tempo_curto = new FuzzyRuleConsequent();
  entao_tempo_curto->addOutput(curto);
  FuzzyRule *R_adequado_seco_quente = new FuzzyRule(9, if_adequado_seco_quente, entao_tempo_curto);
  fuzzy->addFuzzyRule(R_adequado_seco_quente);

// construindo a regra 10: Se umidade_solo = adequado AND umidade_ar = umido AND temperatura = frio THEN tempo_irrigação = nenhum

  FuzzyRuleAntecedent *if_adequado_umido_frio = new FuzzyRuleAntecedent();
  if_adequado_umido_frio->joinWithAND(adequado, umido, frio);
  FuzzyRule *R_adequado_umido_frio = new FuzzyRule(10, if_adequado_umido_frio, entao_tempo_nenhum);
  fuzzy->addFuzzyRule(R_adequado_umido_frio);

// construindo a regra 11: Se umidade_solo = adequado AND umidade_ar = umido AND temperatura = moderada THEN tempo_irrigação = nenhum

  FuzzyRuleAntecedent *if_adequado_umido_moderada = new FuzzyRuleAntecedent();
  if_adequado_umido_moderada->joinWithAND(adequado, umido, moderada);
  FuzzyRule *R_adequado_umido_moderada = new FuzzyRule(11, if_adequado_umido_moderada, entao_tempo_nenhum);
  fuzzy->addFuzzyRule(R_adequado_umido_moderada);

// construindo a regra 12: Se umidade_solo = adequado AND umidade_ar = umido AND temperatura = quente THEN tempo_irrigação = curto

  FuzzyRuleAntecedent *if_adequado_umido_quente = new FuzzyRuleAntecedent();
  if_adequado_umido_quente->joinWithAND(adequado, umido, quente);
  FuzzyRule *R_adequado_umido_quente = new FuzzyRule(12, if_adequado_umido_quente, entao_tempo_curto);
  fuzzy->addFuzzyRule(R_adequado_umido_quente);

// construindo a regra 13: Se umidade_solo = muito_umido AND umidade_ar = seco AND temperatura = frio THEN tempo_irrigação = nenhum

  FuzzyRuleAntecedent *if_muito_umido_seco_frio = new FuzzyRuleAntecedent();
  if_muito_umido_seco_frio->joinWithAND(muito_umido, seco, frio);
  FuzzyRule *R_muito_umido_seco_frio = new FuzzyRule(13, if_muito_umido_seco_frio, entao_tempo_nenhum);
  fuzzy->addFuzzyRule(R_muito_umido_seco_frio);

// construindo a regra 14: Se umidade_solo = muito_umido AND umidade_ar = seco AND temperatura = moderada THEN tempo_irrigação = nenhum

  FuzzyRuleAntecedent *if_muito_umido_seco_moderada = new FuzzyRuleAntecedent();
  if_muito_umido_seco_moderada->joinWithAND(muito_umido, seco, moderada);
  FuzzyRule *R_muito_umido_seco_moderada = new FuzzyRule(14, if_muito_umido_seco_moderada, entao_tempo_nenhum);
  fuzzy->addFuzzyRule(R_muito_umido_seco_moderada);

// construindo a regra 15: Se umidade_solo = muito_umido AND umidade_ar = seco AND temperatura = quente THEN tempo_irrigação = nenhum

  FuzzyRuleAntecedent *if_muito_umido_seco_quente = new FuzzyRuleAntecedent();
  if_muito_umido_seco_quente->joinWithAND(muito_umido, seco, quente);
  FuzzyRule *R_muito_umido_seco_quente = new FuzzyRule(15, if_muito_umido_seco_quente, entao_tempo_nenhum);
  fuzzy->addFuzzyRule(R_muito_umido_seco_quente);

// construindo a regra 16: Se umidade_solo = muito_umido AND umidade_ar = umido AND temperatura = frio THEN tempo_irrigação = nenhum

  FuzzyRuleAntecedent *if_muito_umido_umido_frio = new FuzzyRuleAntecedent();
  if_muito_umido_umido_frio->joinWithAND(muito_umido, umido, frio);
  FuzzyRule *R_muito_umido_umido_frio = new FuzzyRule(16, if_muito_umido_umido_frio, entao_tempo_nenhum);
  fuzzy->addFuzzyRule(R_muito_umido_umido_frio);

// construindo a regra 17: Se umidade_solo = muito_umido AND umidade_ar = umido AND temperatura = moderada THEN tempo_irrigação = nenhum

  FuzzyRuleAntecedent *if_muito_umido_umido_moderada = new FuzzyRuleAntecedent();
  if_muito_umido_umido_moderada->joinWithAND(muito_umido, umido, moderada);
  FuzzyRule *R_muito_umido_umido_moderada = new FuzzyRule(17, if_muito_umido_umido_moderada, entao_tempo_nenhum);
  fuzzy->addFuzzyRule(R_muito_umido_umido_moderada);

// construindo a regra 18: Se umidade_solo = muito_umido AND umidade_ar = umido AND temperatura = quente THEN tempo_irrigação = nenhum

  FuzzyRuleAntecedent *if_muito_umido_umido_quente = new FuzzyRuleAntecedent();
  if_muito_umido_umido_quente->joinWithAND(muito_umido, umido, quente);
  FuzzyRule *R_muito_umido_umido_quente = new FuzzyRule(18, if_muito_umido_umido_quente, entao_tempo_nenhum);
  fuzzy->addFuzzyRule(R_muito_umido_umido_quente);
}

void loop()
{
  digitalWrite(O_IRRIGA, LOW);
  delay(2000);
  //leitura de umidade do ar e temperatura do sensor DHT11
  float umidadeAr = dht.readHumidity();
  float temperatura = dht.readTemperature();

  // Verificação de erros de leitura e informação via terminal
  if (isnan(umidadeAr) || isnan(temperatura)) {
    Serial.println(F("Falha em ler dados do sensor!\n"));
    return;
  }

  //leitura de umidade do solo do sensor HD-38
  float umidadeSolo2 = analogRead(I_UMISOLO);
  float umidadeSolo = map(umidadeSolo2,0,4095,100,0);

  Serial.print("\nValores de entrada: ");
  Serial.print("\n\tTemperatura: ");
  Serial.print(temperatura);
  Serial.print("\n\tUmidade do solo: ");
  Serial.print(umidadeSolo);
  Serial.print("\n\tUmidade do ar: ");
  Serial.print(umidadeAr);

  fuzzy->setInput(1, temperatura);
  fuzzy->setInput(2, umidadeSolo);
  fuzzy->setInput(3, umidadeAr);

  fuzzy->fuzzify();

  Serial.print("\nPertinências para cada termo linguístico: ");
  Serial.print("\n\tTemperatura: \n\t\tFrio-> ");
  Serial.print(frio->getPertinence());
  Serial.print("\n\t\tModerada-> ");
  Serial.print(moderada->getPertinence());
  Serial.print("\n\t\tQuente-> ");
  Serial.print(quente->getPertinence());

  Serial.print("\n\tUmidade do solo: \n\t\tPouco úmido-> ");
  Serial.print(pouco_umido->getPertinence());
  Serial.print("\n\t\tAdequado-> ");
  Serial.print(adequado->getPertinence());
  Serial.print("\n\t\tMuito úmido-> ");
  Serial.print(muito_umido->getPertinence());

  Serial.print("\n\tUmidade do ar: \n\t\tSeco-> ");
  Serial.print(seco->getPertinence());
  Serial.print("\n\t\tUmido-> ");
  Serial.print(umido->getPertinence());

  float tempoIrriga = fuzzy->defuzzify(1)-16.03;

  Serial.print("\nOutput: ");
  Serial.print("\n\tTempo de irrigação: \n\t\tNenhum-> ");
  Serial.print(nenhum->getPertinence());
  Serial.print("\n\t\tCurto-> ");
  Serial.print(curto->getPertinence());
  Serial.print("\n\t\tMédio-> ");
  Serial.print(medio->getPertinence());
  Serial.print("\n\t\tLongo-> ");
  Serial.print(longo->getPertinence());
  Serial.print("\n\t\tMáximo-> ");
  Serial.print(maxi->getPertinence());

  Serial.print("\nResultado: ");
  Serial.print("\n\tTempo de irrigação: \n");
  Serial.print(tempoIrriga);
    if (tempoIrriga > 2) {
  digitalWrite(O_IRRIGA, HIGH);  // HIGH quando há irrigação
  int tempo = tempoIrriga * 1000;
  delay(tempo);
  digitalWrite(O_IRRIGA, LOW);
} else {
  digitalWrite(O_IRRIGA, LOW);   // LOW quando não há irrigação
}

}