namespace generator_web.Models
{
    public class generator_data
    {
        public int Id { get; set; }
        public string CalismaDurumu { get; set; }
        public int OperationMode { get; set; }
        public int SistemCalismaSuresi { get; set; }

        public int SebekeVoltaj_l1 { get; set; }
        public int SebekeVoltaj_l2 { get; set; }
        public int SebekeVoltaj_l3 { get; set; }
        public int SebekeHz { get; set; }
        public int ToplamGuc { get; set; }
        public bool SebekeDurumu { get; set; }

        public int GenVoltaj_l1{ get; set; }
        public int GenVoltaj_l2 { get; set; }
        public int GenVoltaj_l3 { get; set; }
        public int GenHz { get; set; }
        public int GenUretilenGuc{ get; set; }
        public int GenGucFaktoru{ get; set; }
        public int MotorRpm { get; set; }
        public int MotorSicaklik { get; set; }
        public int YagBasinci { get; set; }
        public int YakitSeviyesi { get; set; }
        public int BataryaVoltaji { get; set; }
        public int timestamp { get; set; }

    }
}
