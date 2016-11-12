using System.Collections.Generic;

namespace FlightSearch
{
    public class Airport
    {
        public string kind { get; set; }
        public string code { get; set; }
        public string city { get; set; }
        public string name { get; set; }
    }

    public class City
    {
        public string kind { get; set; }
        public string code { get; set; }
        public string name { get; set; }
    }

    public class Aircraft
    {
        public string kind { get; set; }
        public string code { get; set; }
        public string name { get; set; }
    }

    public class Tax
    {
        public string kind { get; set; }
        public string id { get; set; }
        public string name { get; set; }
    }

    public class Carrier
    {
        public string kind { get; set; }
        public string code { get; set; }
        public string name { get; set; }
    }

    public class Data
    {
        public string kind { get; set; }
        public List<Airport> airport { get; set; }
        public List<City> city { get; set; }
        public List<Aircraft> aircraft { get; set; }
        public List<Tax> tax { get; set; }
        public List<Carrier> carrier { get; set; }
    }

    public class Flight
    {
        public string carrier { get; set; }
        public string number { get; set; }
    }

    public class Leg
    {
        public string kind { get; set; }
        public string id { get; set; }
        public string aircraft { get; set; }
        public string arrivalTime { get; set; }
        public string departureTime { get; set; }
        public string origin { get; set; }
        public string destination { get; set; }
        public string originTerminal { get; set; }
        public int duration { get; set; }
        public int onTimePerformance { get; set; }
        public int mileage { get; set; }
        public bool secure { get; set; }
        public string destinationTerminal { get; set; }
        public string meal { get; set; }
        public string operatingDisclosure { get; set; }
        public int? connectionDuration { get; set; }
        public bool? changePlane { get; set; }
    }

    public class Segment
    {
        public string kind { get; set; }
        public int duration { get; set; }
        public Flight flight { get; set; }
        public string id { get; set; }
        public string cabin { get; set; }
        public string bookingCode { get; set; }
        public int bookingCodeCount { get; set; }
        public string marriedSegmentGroup { get; set; }
        public List<Leg> leg { get; set; }
        public int connectionDuration { get; set; }
    }

    public class Slouse
    {
        public string kind { get; set; }
        public int duration { get; set; }
        public List<Segment> segment { get; set; }
    }

    public class Fare
    {
        public string kind { get; set; }
        public string id { get; set; }
        public string carrier { get; set; }
        public string origin { get; set; }
        public string destination { get; set; }
        public string basisCode { get; set; }
    }

    public class BagDescriptor
    {
        public string kind { get; set; }
        public string commercialName { get; set; }
        public int count { get; set; }
        public string subcode { get; set; }
        public List<string> description { get; set; }
    }

    public class FreeBaggageOption
    {
        public string kind { get; set; }
        public List<BagDescriptor> bagDescriptor { get; set; }
        public int pieces { get; set; }
    }

    public class SegmentPricing
    {
        public string kind { get; set; }
        public string fareId { get; set; }
        public string segmentId { get; set; }
        public List<FreeBaggageOption> freeBaggageOption { get; set; }
    }

    public class Passengers
    {
        public string kind { get; set; }
        public int adultCount { get; set; }
    }

    public class Tax2
    {
        public string kind { get; set; }
        public string id { get; set; }
        public string chargeType { get; set; }
        public string code { get; set; }
        public string country { get; set; }
        public string salePrice { get; set; }
    }

    public class Pricing
    {
        public string kind { get; set; }
        public List<Fare> fare { get; set; }
        public List<SegmentPricing> segmentPricing { get; set; }
        public string baseFareTotal { get; set; }
        public string saleFareTotal { get; set; }
        public string saleTaxTotal { get; set; }
        public string saleTotal { get; set; }
        public Passengers passengers { get; set; }
        public List<Tax2> tax { get; set; }
        public string fareCalculation { get; set; }
        public string latestTicketingTime { get; set; }
        public string ptc { get; set; }
    }

    public class TripOption
    {
        public string kind { get; set; }
        public string saleTotal { get; set; }
        public string id { get; set; }
        public List<Slouse> slice { get; set; }
        public List<Pricing> pricing { get; set; }
    }

    public class Trips
    {
        public string kind { get; set; }
        public string requestId { get; set; }
        public Data data { get; set; }
        public List<TripOption> tripOption { get; set; }
    }

    public class RootObject
    {
        public string kind { get; set; }
        public Trips trips { get; set; }
    }
}
