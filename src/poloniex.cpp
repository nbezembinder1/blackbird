#include <string.h>
#include <iostream>
#include <unistd.h>
#include <sstream>
#include <math.h>
#include <sys/time.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include "base64.h"
#include <jansson.h>
#include "poloniex.h"
#include "curl_fun.h"

namespace Poloniex {

double getQuote(Parameters& params, bool isBid) {
  bool GETRequest = false;
  json_t* root = getJsonFromUrl(params, "https://poloniex.com/public?command=returnTicker", "", GETRequest);
  const char* quote;
  double quoteValue;
  if (isBid) {
    quote = json_string_value(json_object_get(json_object_get(root, "USDT_BTC"), "highestBid"));
  } else {
    quote = json_string_value(json_object_get(json_object_get(root, "USDT_BTC"), "lowestAsk"));
  }
  if (quote != NULL) {
    quoteValue = atof(quote);
  } else {
    quoteValue = 0.0;
  }
  json_decref(root);
  return quoteValue;
}

double getAvail(Parameters& params, std::string currency) {
  // TODO Function is implemented, but still needs to be tested
  json_t* root = authRequest(params, "https://poloniex.com/tradingApi", "returnBalances", "");
  double availability;
  if (currency.compare("usd") == 0) {
	availability = atof(json_string_value(json_object_get(root, "USDT")));
  } else if (currency.compare("btc") == 0) {
	availability = atof(json_string_value(json_object_get(root, "BTC")));
  } else {
	availability = 0.0;
  }
  json_decref(root);
  return availability;
}

int sendLongOrder(Parameters& params, std::string direction, double quantity, double price) {
  // TODO
  // This function needs to be implemented
  // The code below is given as a general guideline but needs to be rewritten
  // to match the Poloniex API
  *params.logFile << "<Poloniex> Trying to send a \"" << direction << "\" limit order: " << quantity << "@$" << price << "..." << std::endl;
  std::ostringstream oss;
  std::string options = oss.str();
  json_t* root = authRequest(params, "https://poloniex.com/tradingApi", "marginBuy", options);
  int orderId = json_integer_value(json_object_get(root, "order_id"));
  *params.logFile << "<Poloniex> Done (order ID: " << orderId << ")\n" << std::endl;
  json_decref(root);
  return orderId;
}

int sendShortOrder(Parameters& params, std::string direction, double quantity, double price) {
  // TODO
  // This function needs to be implemented
  // The code below is given as a general guideline but needs to be rewritten
  // to match the Poloniex API
  *params.logFile << "<Poloniex> Trying to send a \"" << direction << "\" limit order: " << quantity << "@$" << price << "..." << std::endl;
  std::ostringstream oss;
  std::string options = oss.str();
  json_t* root = authRequest(params, "https://poloniex.com/tradingApi", "marginSell", options);
  int orderId = json_integer_value(json_object_get(root, "order_id"));
  *params.logFile << "<Poloniex> Done (order ID: " << orderId << ")\n" << std::endl;
  json_decref(root);
  return orderId;
}

bool isOrderComplete(Parameters& params, int orderId) {
  // TODO
  // This function needs to be implemented
  // The code below is given as a general guideline but needs to be rewritten
  // to match the Poloniex API
  if (orderId == 0) {
    return true;
  }
  std::ostringstream oss;
  oss << "\"order_id\":" << orderId;
  std::string options = oss.str();
  json_t* root = authRequest(params, "https://poloniex.com/tradingApi", "order/status", options);
  bool isComplete = !json_boolean_value(json_object_get(root, "is_live"));
  json_decref(root);
  return isComplete;
}

double getActivePos(Parameters& params) {
  // TODO
  // This function needs to be implemented
  // The code below is given as a general guideline but needs to be rewritten
  // to match the Poloniex API
  json_t* root = authRequest(params, "https://poloniex.com/tradingApi", "positions", "");
  double position;
  if (json_array_size(root) == 0) {
    *params.logFile << "<Poloniex> WARNING: BTC position not available, return 0.0" << std::endl;
    position = 0.0;
  } else {
    position = atof(json_string_value(json_object_get(json_array_get(root, 0), "amount")));
  }
  json_decref(root);
  return position;
}

double getLimitPrice(Parameters& params, double volume, bool isBid) {
  // TODO
  // This function needs to be implemented
  // The code below is given as a general guideline but needs to be rewritten
  // to match the Poloniex API
  bool GETRequest = false;
  json_t* root;
  if (isBid) {
    root = json_object_get(getJsonFromUrl(params, "", "", GETRequest), "bids");
  } else {
    root = json_object_get(getJsonFromUrl(params, "", "", GETRequest), "asks");
  }
  *params.logFile << "<Poloniex> Looking for a limit price to fill " << fabs(volume) << " BTC..." << std::endl;
  double tmpVol = 0.0;
  double p;
  double v;
  int i = 0;
    // loop on volume
  while (tmpVol < fabs(volume) * params.orderBookFactor) {
    p = atof("");
    v = atof("");
    *params.logFile << "<Poloniex> order book: " << v << "@$" << p << std::endl;
    tmpVol += v;
    i++;
  }
  double limPrice = 0.0;
  limPrice = atof(json_string_value(json_object_get(json_array_get(root, i-1), "price")));
  json_decref(root);
  return limPrice;
}

json_t* authRequest(Parameters& params, std::string url, std::string request, std::string options) {
  // TODO Function is implemented, but still needs to be tested

  // Generate nonce
  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long nonce = (tv.tv_sec * 100000.0) + (tv.tv_usec * 0.001) + 0.5;

  // Sign POST data using the key's "secret" according to the HMAC-SHA512 method
  std::ostringstream oss;
  oss << "command=" << request << "&nonce=" << nonce;

  unsigned char* digest;
  digest = HMAC(EVP_sha512(), params.poloniexSecret.c_str(), strlen(params.poloniexSecret.c_str()), (unsigned char*)oss.str().c_str(), strlen(oss.str().c_str()), NULL, NULL);
  char mdString[SHA512_DIGEST_LENGTH+100];  // FIXME +100
  for (int i = 0; i < SHA512_DIGEST_LENGTH; ++i) {
    sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
  }

  // Set POST parameters using request, nonce and option values
  oss.clear();
  oss.str("");
  oss << "command=" << request << "&nonce=" << nonce;

  // Poloniex throws an error if '&' is appended as last character
  if(!options.empty())
	  oss << "&" << options;

  std::string postParams = oss.str().c_str();

  // Add Key and generated signature as HTTP Headers
  struct curl_slist *headers = NULL;
  std::ostringstream key, sign;
  key << "Key:" << params.poloniexApi;
  sign << "Sign:" << mdString;
  headers = curl_slist_append(headers, key.str().c_str());
  headers = curl_slist_append(headers, sign.str().c_str());

  CURLcode resCurl;
  if (params.curl) {
    std::string readBuffer;
    curl_easy_setopt(params.curl, CURLOPT_POST,1L);
    curl_easy_setopt(params.curl, CURLOPT_PROXY, "http://127.0.0.1:8888/"); // TODO: Debugging only
    curl_easy_setopt(params.curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(params.curl, CURLOPT_POSTFIELDS, postParams.c_str());
    curl_easy_setopt(params.curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(params.curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(params.curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(params.curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(params.curl, CURLOPT_CONNECTTIMEOUT, 10L);
    resCurl = curl_easy_perform(params.curl);
    json_t* root;
    json_error_t error;
    while (resCurl != CURLE_OK) {
      *params.logFile << "<Poloniex> Error with cURL. Retry in 2 sec..." << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
    }
    root = json_loads(readBuffer.c_str(), 0, &error);
    while (!root) {
      *params.logFile << "<Poloniex> Error with JSON:\n" << error.text << std::endl;
      *params.logFile << "<Poloniex> Buffer:\n" << readBuffer.c_str() << std::endl;
      *params.logFile << "<Poloniex> Retrying..." << std::endl;
      sleep(2.0);
      readBuffer = "";
      resCurl = curl_easy_perform(params.curl);
      while (resCurl != CURLE_OK) {
        *params.logFile << "<Poloniex> Error with cURL. Retry in 2 sec..." << std::endl;
        sleep(2.0);
        readBuffer = "";
        resCurl = curl_easy_perform(params.curl);
      }
      root = json_loads(readBuffer.c_str(), 0, &error);
    }
    curl_easy_reset(params.curl);
    return root;
  } else {
    *params.logFile << "<Poloniex> Error with cURL init." << std::endl;
    return NULL;
  }
}

}

