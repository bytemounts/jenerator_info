namespace generator_web.Models
{
    public class Alert
    {
        public int Id { get; set; }
        public string Type { get; set; } // "warning", "error", "info"
        public string Message { get; set; }
        public DateTime CreatedAt { get; set; }
        public bool IsActive { get; set; }
    }
}
