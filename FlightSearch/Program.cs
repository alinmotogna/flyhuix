using System;
using System.Net;
using System.IO;
using System.Web;
using System.Collections.Generic;
using Newtonsoft.Json;

namespace FlightSearch
{
    class Program
    {
        // Demo code for QPX Express API
        // Free requests limited to 50 per day
        // Billing must be activated at https://console.developers.google.com/ to allow more
        static void Main(string[] args)
        {
            #region parameters
            // API key needs to be generated at https://console.developers.google.com/
            // Add a new project, add the QPX Express API to it, and generate a server key
            // For security, you can specify only requests allowed from certain IPs
            string API_KEY = "AIzaSyDLo2QRtu4g-k-hPUv3kLRY5c6dDMWOv_0";
            string URL = "https://www.googleapis.com/qpxExpress/v1/trips/search?key=" + API_KEY;

            // one or more required
            int adultCount = 1;
            int childCount = 0;
            int infantInLapCount = 0;
            int infantInSeatCount = 0;
            int seniorCount = 0;

            // required
            string origin = "ELP";
            string destination = "IAH";
            string date = "2015-02-05";
            bool refundable = true;
            int solutions = 50;

            // optional
            int maxStops = 2;
            string earliestDepartureTime = null;
            string latestDepartureTime = null;
            #endregion

            #region craft JSON
            string json = @"{""request"": {""passengers"": {";
            if (adultCount > 0)
                json += @"""adultCount"": " + adultCount;
            if (childCount > 0)
                json += @",""childCount"": " + childCount;
            if (infantInLapCount > 0)
                json += @",""infantInLapCount"": " + infantInLapCount;
            if (infantInSeatCount > 0)
                json += @",""infantInSeatCount"": " + infantInSeatCount;
            if (seniorCount > 0)
                json += @",""seniorCount"": " + seniorCount;
            json += @"},""slice"": [{";
            json += @"""origin"": """ + origin + @"""";
            json += @",""destination"": """ + destination + @"""";
            json += @",""date"": """ + date + @"""";
            if (maxStops > 0)
                json += @",""maxStops"": " + maxStops;
            if (earliestDepartureTime != null || latestDepartureTime != null)
                json += @",""permittedDepartureTime"": {";
            if (earliestDepartureTime != null)
                json += @"""earliestTime"": """ + earliestDepartureTime + @"""";
            if (latestDepartureTime != null)
                json += @",""latestTime"": """ + latestDepartureTime + @"""";
            if (earliestDepartureTime != null || latestDepartureTime != null)
                json += "}";
            json += @"}],";
            json += @"""refundable"": " + refundable.ToString().ToLower() + ",";
            if (solutions > 0)
                json += @"""solutions"": " + solutions;
            json += "}}";
            // Console.WriteLine(json);
            #endregion
            #region make POST
            // make a POST to the proper site with the JSON
            var httpWebRequest = (HttpWebRequest)WebRequest.Create(URL);
            httpWebRequest.ContentType = "application/json";
            httpWebRequest.Method = "POST";
            var result = "";
            using (var streamWriter = new StreamWriter(httpWebRequest.GetRequestStream()))
            {
                streamWriter.Write(json);
                streamWriter.Flush();
                streamWriter.Close();

                var httpResponse = (HttpWebResponse)httpWebRequest.GetResponse();
                using (var streamReader = new StreamReader(httpResponse.GetResponseStream()))
                {
                    result = streamReader.ReadToEnd();

                }
            }
            #endregion
            #region print results to file and console
            // System.IO.File.WriteAllText("result.txt", result);
            //Console.WriteLine(result);
            System.IO.File.WriteAllText("result.txt", "");
            RootObject res = new RootObject();
            res = JsonConvert.DeserializeObject<RootObject>(result);
            int counter = 1;
            foreach (TripOption op in res.trips.tripOption)
            {
                WriteLine("Trip Option " + counter);
                WriteLine("Total Price: " + op.saleTotal);
                int counter2 = 1;
                foreach (Slouse slice in op.slice)
                {
                    foreach (Segment seg in slice.segment)
                    {
                        string carrierID = seg.flight.carrier;
                        string carrier = "";
                        string number = seg.flight.number;
                        foreach (Carrier carr in res.trips.data.carrier)
                        {
                            if (carr.code.Equals(carrierID))
                                carrier = carr.name;
                        }
                        int seats = seg.bookingCodeCount;
                        foreach (Leg leg in seg.leg)
                        {
                            WriteLine("Flight " + counter2 + ":\t\t" + carrierID + " " + number);
                            counter2++;
                            WriteLine("Origin: " + leg.origin + "\t\tTerminal: " + leg.originTerminal);
                            WriteLine("Departure Time: " + DateTime.Parse(leg.departureTime).ToString("g"));
                            WriteLine("Destination: " + leg.destination + "\tTerminal: " + leg.destinationTerminal);
                            WriteLine("Arrival Time: " + DateTime.Parse(leg.arrivalTime).ToString("g"));
                            WriteLine("Carrier: " + carrier);
                            WriteLine("Duration: " + leg.duration + " minutes");
                            WriteLine("Seats Available: " + seats);
                        }
                    }
                }
                WriteLine("");
                counter++;
            }
            Console.ReadKey();
            #endregion
        }
        public static void WriteLine(string str)
        {
            Console.WriteLine(str);
            System.IO.File.AppendAllText("result.txt", str + "\n");
        }
    }
}